//
// Created by lining on 2/15/22.
//

#include <fcntl.h>
#include <iostream>
#include <log/Log.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <valarray>
#include <iomanip>
#include "server/FusionServer.h"
#include "merge/merge.h"
#include "merge/mergeStruct.h"
#include "simpleini/SimpleIni.h"
#include "sqlite3.h"
#include "timeTask.hpp"

using namespace z_log;

FusionServer::FusionServer(bool isMerge) {
    this->isRun.store(false);
    this->isMerge = isMerge;
}

FusionServer::FusionServer(uint16_t port, string config, int maxListen, bool isMerge) {
    this->port = port;
    this->config = config;
    this->maxListen = maxListen;
    this->isRun.store(false);
    this->isMerge = isMerge;
}

FusionServer::~FusionServer() {
    Close();
}

void FusionServer::initConfig() {

    if (!config.empty()) {
        FILE *fp = fopen(config.c_str(), "r+");
        if (fp == nullptr) {
            Error("can not open file:%s", config.c_str());
            return;
        }
        CSimpleIniA ini;
        ini.LoadFile(fp);

        const char *S_repateX = ini.GetValue("server", "repateX", "");
        repateX = atof(S_repateX);
//        const char *S_widthX = ini.GetValue("server", "widthX", "");
//        widthX = atof(S_widthX);
//        const char *S_widthY = ini.GetValue("server", "widthY", "");
//        widthY = atof(S_widthY);
//        const char *S_Xmax = ini.GetValue("server", "Xmax", "");
//        Xmax = atof(S_Xmax);
//        const char *S_Ymax = ini.GetValue("server", "Ymax", "");
//        Ymax = atof(S_Ymax);
        const char *S_gatetx = ini.GetValue("server", "gatetx", "");
        gatetx = atof(S_gatetx);
        const char *S_gatety = ini.GetValue("server", "gatety", "");
        gatety = atof(S_gatety);
        const char *S_gatex = ini.GetValue("server", "gatex", "");
        gatex = atof(S_gatex);
        const char *S_gatey = ini.GetValue("server", "gatey", "");
        gatey = atof(S_gatey);

        fclose(fp);
    }
}


static int CallbackGetCL_ParkingArea(void *data, int argc, char **argv, char **azColName) {
    string colName;
    if (data != nullptr) {
        auto server = (FusionServer *) data;
        for (int i = 0; i < argc; i++) {
            colName = string(azColName[i]);
            if (colName.compare("UName") == 0) {
                server->matrixNo = string(argv[i]);
                cout << "get matrixNo from db:" + server->matrixNo << endl;
            }
        }
    }

    return 0;
}


void FusionServer::getMatrixNoFromDb() {
    sqlite3 *db;
    char *errmsg = nullptr;

    string dbName;
#ifdef arm64
    dbName = "/home/nvidianx/bin/CLParking.db";
#else
    dbName = "./CLParking.db";
#endif


    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
    }

    //base
    char *sqlCmd = "select * from CL_ParkingArea";
    rc = sqlite3_exec(db, sqlCmd, CallbackGetCL_ParkingArea, this, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
    }

}

static int CallbackGetbelong_intersection(void *data, int argc, char **argv, char **azColName) {
    string colName;
    if (data != nullptr) {
        auto server = (FusionServer *) data;
        for (int i = 0; i < argc; i++) {
            colName = string(azColName[i]);
            if (colName.compare("PlatId") == 0) {
                server->watchDatasCrossID = string(argv[i]);
                cout << "get crossID from db:" + server->watchDatasCrossID << endl;
            }
        }
    }

    return 0;
}

void FusionServer::getCrossIdFromDb() {
    sqlite3 *db;
    char *errmsg = nullptr;

    string dbName;
#ifdef arm64
    dbName = "/home/nvidianx/eoc_configure.db";
#else
    dbName = "./eoc_configure.db";
#endif


    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
    }

    //intersectionEntity
    char *sqlCmd = "select * from belong_intersection";
    rc = sqlite3_exec(db, sqlCmd, CallbackGetbelong_intersection, this, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
    }
}


int FusionServer::Open() {
    //1.申请sock
    sock = socket(AF_INET, SOCK_STREAM /*| SOCK_NONBLOCK*/, IPPROTO_TCP);
    if (sock == -1) {
        Fatal("server sock fail:%s", strerror(errno));
        return -1;
    }
    //2.设置地址、端口
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(this->port);
    //将套接字绑定到服务器的网络地址上
    if (bind(sock, (struct sockaddr *) &server_addr, (socklen_t) sizeof(server_addr)) < 0) {
        Fatal("server sock bind error:%s", strerror(errno));
        close(sock);
        sock = 0;
        return -1;
    }

    //3.设置属性
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//    struct timeval timeout;
//    timeout.tv_sec = 3;
//    timeout.tv_usec = 0;
//    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
//    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

//    int recvBufSize = 4 * 1024 * 1024;
//    int sendBufSize = 4 * 1024 * 1024;
//    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
//    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));

    //4.监听
    if (listen(sock, maxListen) != 0) {
        Error("server sock listen fail,error:%s", strerror(errno));
        close(sock);
        sock = 0;
        return -1;
    }

    //5.创建一个epoll句柄
    epoll_fd = epoll_create(MAX_EVENTS);
    if (epoll_fd == -1) {
        Error("epoll create fail");
        close(sock);
        sock = 0;
        return -1;
    }
    ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    ev.data.fd = sock;
    // 向epoll注册server_sockfd监听事件
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) == -1) {
        Error("epoll_ctl server fd failed:%s", strerror(errno));
        close(sock);
        sock = 0;
        return -1;
    }

    Info("server sock %d create success", sock);

    isRun.store(true);
    return 0;
}

int FusionServer::Run() {

    if (sock <= 0) {
        Error("server sock:%d", sock);
        return -1;
    }
    if (!isRun) {
        return -1;
    }

    //获取matrixNo
    getMatrixNoFromDb();
    //获取路口编号
//    getCrossIdFromDb();

    //开启服务器监视客户端线程
    threadMonitor = thread(ThreadMonitor, this);
    pthread_setname_np(threadMonitor.native_handle(), "FusionServer monitor");
    threadMonitor.detach();

    if (isMerge) {
        //开启服务器多路数据融合线程
        threadMerge = thread(ThreadMerge, this);
        pthread_setname_np(threadMerge.native_handle(), "FusionServer merge");
        threadMerge.detach();
    } else {
        //开启服务器多路数据不融合线程
        threadNotMerge = thread(ThreadNotMerge, this);
        pthread_setname_np(threadNotMerge.native_handle(), "FusionServer notMerge");
        threadNotMerge.detach();
    }

    //开启定时任务
    threadTimerTask = thread(ThreadTimerTask, this);
    pthread_setname_np(threadTimerTask.native_handle(), "FusionServer threadTimerTask");
    threadTimerTask.detach();

    return 0;
}

int FusionServer::Close() {
    if (sock > 0) {
        close(sock);
        sock = 0;
    }
    DeleteAllClient();

    isRun.store(false);

    return 0;
}

int FusionServer::setNonblock(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int FusionServer::AddClient(int client_sock, struct sockaddr_in remote_addr) {

    //多线程处理，不应该设置非阻塞模式
//    setNonblock(client_sock);
    // epoll add
    this->ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLRDHUP;
    this->ev.data.fd = client_sock;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, client_sock, &this->ev) == -1) {
        Error("epoll add:%d fail,%s", client_sock, strerror(errno));
        return -1;
    }
    //add vector
    pthread_mutex_lock(&this->lockVectorClient);
    //根据ip，看下是否原来有记录，有的话，清除后在添加
    string addIp = string(inet_ntoa(remote_addr.sin_addr));
    for (int i = 0; i < this->vectorClient.size(); i++) {
        auto iter = this->vectorClient.at(i);
        string curIp = string(inet_ntoa(iter->clientAddr.sin_addr));
        if (curIp == addIp) {
            delete vectorClient.at(i);
            this->vectorClient.erase(this->vectorClient.begin() + i);
            this->vectorClient.shrink_to_fit();
            Info("remove repeat ip:%s", curIp.c_str());
        }
    }
    //clientInfo
    int index = this->vectorClient.size();
    ClientInfo *clientInfo = new ClientInfo(remote_addr, client_sock, this, index);

    this->vectorClient.push_back(clientInfo);
    pthread_mutex_unlock(&this->lockVectorClient);

    Info("add client %d success,ip:%s", client_sock, addIp.c_str());

    //开启客户端处理线程
    clientInfo->ProcessRecv();

    return 0;
}

int FusionServer::RemoveClient(int client_sock) {

    //epoll del
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_sock, &this->ev) == -1) {
        Error("epoll del %d fail,%s", client_sock, strerror(errno));
        //epoll失败，但是还是要从数组剔除
    }
    pthread_mutex_lock(&this->lockVectorClient);
    //vector erase
    for (int i = 0; i < vectorClient.size(); i++) {
        auto iter = vectorClient.at(i);
        if (iter->sock == client_sock) {

            //delete client class  close sock release clientInfo->rb
            delete vectorClient.at(i);

            //erase vector
            vectorClient.erase(vectorClient.begin() + i);
            vectorClient.shrink_to_fit();

            Info("remove client:%d", client_sock);
        }
    }
    pthread_mutex_unlock(&this->lockVectorClient);

    return 0;
}

int FusionServer::DeleteAllClient() {

    //删除客户端数组
    if (!vectorClient.empty()) {

        pthread_mutex_lock(&lockVectorClient);

        for (auto iter:vectorClient) {
            if (iter->sock > 0) {
                close(iter->sock);
            }
            delete iter;
        }
        vectorClient.clear();
        pthread_mutex_unlock(&lockVectorClient);
    }

    return 0;
}

void FusionServer::ThreadMonitor(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    int nfds = 0;// epoll监听事件发生的个数
    struct sockaddr_in remote_addr; // 客户端网络地址结构体
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int client_fd;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {
        // 等待事件发生
        nfds = epoll_wait(server->epoll_fd, server->wait_events, MAX_EVENTS, -1);

        if (nfds == -1) {
            continue;
        }
        for (int i = 0; i < nfds; i++) {
            //外部只处理连接和断开状态
            if ((server->wait_events[i].data.fd == server->sock) &&
                ((server->wait_events[i].events & EPOLLIN) == EPOLLIN)) {
                // 客户端有新的连接请求
                // 等待客户端连接请求到达
                client_fd = accept(server->sock, (struct sockaddr *) &remote_addr, &sin_size);
                if (client_fd < 0) {
                    Error("server accept fail:%s", strerror(errno));
                    continue;
                }
                Info("accept client:%d,ip:%s", client_fd, inet_ntoa(remote_addr.sin_addr));
                //加入客户端数组，开启客户端处理线程
                server->AddClient(client_fd, remote_addr);

            } else if ((server->wait_events[i].data.fd > 2) &&
                       ((server->wait_events[i].events & EPOLLERR) ||
                        (server->wait_events[i].events & EPOLLRDHUP) ||
                        (server->wait_events[i].events & EPOLLOUT))) {
                //有异常发生 close client

                for (int j = 0; j < server->vectorClient.size(); j++) {
                    auto iter = server->vectorClient.at(j);
                    if (iter->sock == server->wait_events[i].data.fd) {
                        //抛出一个对方已关闭的异常，等待检测线程进行处理
                        server->RemoveClient(iter->sock);
                        break;
                    }
                }
            }
        }
    }

    close(server->epoll_fd);
    close(server->sock);
    server->sock = 0;

    Info("%s exit", __FUNCTION__);
}

void FusionServer::ThreadCheck(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {
        sleep(1);

        if (!server->isRun) {
            continue;
        }
        //检查客户端数组状态，
        // 主要是接收时间receive_time receive_time跟现在大于150秒则断开链接
        // needRelease上抛的释放标志，设置为真后，等待队列全部为空后进行释放
        if (server->vectorClient.empty()) {
            continue;
        }
//        cout << "当前客户端数量：" << to_string(server->vectorClient.size()) << endl;

        //切记，这里删除客户端的时候有加锁解锁操作，所以最外围不要加锁！！！
//        pthread_mutex_lock(&server->lockVectorClient);

        for (auto iter: server->vectorClient) {

//            //检查超时
//            if (iter->isReceive_timeSet) {
//                timeval tv;
//                pthread_mutex_lock(&iter->lock_receive_time);
//
//                gettimeofday(&tv, nullptr);
//                if ((tv.tv_sec > iter->receive_time.tv_sec) &&
//                    (tv.tv_sec - iter->receive_time.tv_sec) >= server->checkStatusTimeval.tv_sec) {
//                    Info("client-%d检测超时", iter->sock);
//                    iter->needRelease.store(true);
//                }
//
//                pthread_mutex_unlock(&iter->lock_receive_time);
//
//            }
            //检查needRelease
            {
                if (iter->needRelease.load()) {
                    server->RemoveClient(iter->sock);
                }
            }
        }

//        pthread_mutex_unlock(&server->lockVectorClient);

    }

    Info("%s exit", __FUNCTION__);

}

void FusionServer::ThreadTimerTask(void *pServer) {

    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    Info("%s run", __FUNCTION__);

    vector<Timer> timers;

    Timer timerFusionData;
    timerFusionData.start(100, std::bind(TaskFindOneFrame_FusionData, server, 10));
    timers.push_back(timerFusionData);


    Timer timerTrafficFlow;
    timerTrafficFlow.start(1000, std::bind(TaskFindOneFrame_TrafficFlow, server, 3));
    timers.push_back(timerTrafficFlow);

    Timer timerLineupInfo;
    timerLineupInfo.start(1000, std::bind(TaskFindOneFrame_LineupInfo, server, 3));
    timers.push_back(timerLineupInfo);


    Timer timerCrossTrafficJamAlarm;
    timerCrossTrafficJamAlarm.start(1000, std::bind(TaskFindOneFrame_CrossTrafficJamAlarm, server, 10));
    timers.push_back(timerCrossTrafficJamAlarm);

    Timer timerMultiViewCarTracks;
    timerMultiViewCarTracks.start(66, std::bind(TaskFindOneFrame_MultiViewCarTracks, server, 10));
    timers.push_back(timerMultiViewCarTracks);

    while (server->isRun.load()) {
        sleep(1);
    }
    for (auto &iter: timers) {
        iter.stop();
    }
    Info("%s exit", __FUNCTION__);


}


void FusionServer::TaskFindOneFrame_FusionData(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    if (server->isRun.load()) {

        //所有的客户端都要缓存3帧数据
        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex;
        for (int i = 0; i < ARRAY_SIZE(server->watchDatas); i++) {
            auto iter = server->watchDatas[i];
//            Info("client-%d,watchData size:%d", iter->sock, iter->queueWatchData.size());
            //得到最大帧数
            if (iter.size() > maxPkgs) {
                maxPkgs = iter.size();
                maxPkgsIndex = i;
            }
        }
//        Info("clientNum %d,maxPkgs %d", clientNum, maxPkgs);
        if (maxPkgs >= cache) {
            Info("寻找同一帧数据");
            OBJS objs;//容量为4,依次是北、东、南、西
            //2.确定时间戳
            WatchData refer;
            if (server->watchDatas[maxPkgsIndex].front(refer)) {
                server->watchDatasCurTimestamp = refer.timstamp;//一直取第一路的时间戳为基准
            } else {
                server->watchDatasCurTimestamp += 100;
                Info("时间戳自动加100");
            }

            //如果现在的时间戳比本地的系统时间差2分钟，则用本地时间-1分钟的标准
            struct timeval now;
            gettimeofday(&now, nullptr);
            uint64_t timestamp = (now.tv_sec * 1000 + now.tv_usec / 1000) - 1 * 60 * 1000;
            if (server->watchDatasCurTimestamp < timestamp) {
                Info("选取的系统时间-1分钟");
                server->watchDatasCurTimestamp = timestamp;
            }

            Info("这次选取的时间戳标准为%lu", server->watchDatasCurTimestamp);


            //3取数
            auto obj = objs.roadData;
            //3.1清理
            for (int i = 0; i < ARRAY_SIZE(obj); i++) {
                obj[i].imageData = "";
                obj[i].hardCode = "";
                obj[i].listObjs.clear();
            }
            //清理第N路的取帧时间戳记录
            for (int i = 0; i < ARRAY_SIZE(server->watchDatasXRoadTimestamp); i++) {
                server->watchDatasXRoadTimestamp[i] = 0;
            }
            //测量本次内时间差用的临时变量
            uint64_t max = 0;
            uint64_t min = 0;

            //3.2取数据
            int getDataRoadNum = 0;//获取了几路数据
            for (int i = 0; i < ARRAY_SIZE(server->watchDatas); i++) {
                auto iter = server->watchDatas[i];
                //watchData数据
                if (iter.empty()) {
                    //客户端未有数据传入，不填充数据
                    Info("第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        WatchData refer;
                        if (iter.front(refer)) {
                            if ((refer.timstamp >=
                                 (double) (server->watchDatasCurTimestamp - server->watchDatasThresholdFrame)) &&
                                (refer.timstamp <=
                                 (double) (server->watchDatasCurTimestamp + server->watchDatasThresholdFrame))) {
                                //出队列
                                WatchData cur;
                                iter.pop(cur);
                                //按路记录时间戳
                                server->watchDatasXRoadTimestamp[i] = cur.timstamp;
                                Info("第%d路取的时间戳是%lu", i + 1, server->watchDatasXRoadTimestamp[i]);

                                //get max min
                                if (max == 0) {
                                    max = (uint64_t) cur.timstamp;
                                } else {
                                    max = (uint64_t) cur.timstamp > max ? (uint64_t) cur.timstamp : max;
                                }

                                if (min == 0) {
                                    min = (uint64_t) cur.timstamp;
                                } else {
                                    min = (uint64_t) cur.timstamp < min ? (uint64_t) cur.timstamp : min;
                                }


                                //TODO 赋值路口编号
                                if (server->watchDatasCrossID.empty()) {
                                    server->watchDatasCrossID = cur.matrixNo;
                                }
                                //时间戳大于当前时间戳，直接取

                                obj[i].hardCode = cur.hardCode;
                                obj[i].imageData = cur.imageData;
//                                    Info("寻找同一帧，找到第%d路的数据后，hardCode:%s 图像大小%d", i, obj[i].hardCode.c_str(),
//                                         obj[i].imageData.size());
                                for (auto item: cur.lstObjTarget) {
                                    OBJECT_INFO_T objectInfoT;
                                    //转换
                                    ObjTarget2OBJECT_INFO_T(item, objectInfoT);
                                    obj[i].listObjs.push_back(objectInfoT);
                                }

                                getDataRoadNum++;
                                isFind = true;
                            } else if ((refer.timstamp <
                                        (double) (server->watchDatasCurTimestamp - server->watchDatasThresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
                                Info("第%d路时间戳%lu靠前,舍弃", i + 1, (uint64_t) refer.timstamp);
                                //出队列抛弃
                                WatchData cur;
                                iter.pop(cur);

                            } else if ((refer.timstamp >
                                        (double) (server->watchDatasCurTimestamp + server->watchDatasThresholdFrame))) {
                                Info("第%d路时间戳%lu靠后,保留", i + 1, (uint64_t) refer.timstamp);
                                isFind = true;
                            }
                        }
                        //如果全部抛完
                        if (iter.empty()) {
                            //队列为空，直接退出
                            Info("第%d路队列为空直接退出", i + 1);
                            isFind = true;
                        }
                    } while (!isFind);
                }
            }

            //        Info("max %lu,min %lu", max, min);
            server->curTimeDistance = (max - min);
            if (server->timeDistance == 0) {
                server->timeDistance = server->curTimeDistance;
            } else {
                server->timeDistance = server->curTimeDistance > server->timeDistance ?
                                       server->curTimeDistance : server->timeDistance;
            }

            Info("当前帧时间差为%lu,统计最大时间差为%lu", server->curTimeDistance, server->timeDistance);

            //3.存入待融合队列，即queueObjs
            if (getDataRoadNum == 1) {
                objs.isOneRoad = true;

                for (int i = 0; i < ARRAY_SIZE(obj); i++) {
                    if (!obj[i].listObjs.empty() || !obj[i].imageData.empty()) {
                        objs.hasDataRoadIndex = i;
//                        Info("hasDataRoadIndex:%d", objs.hasDataRoadIndex);
                        break;
                    }
                }
            } else {
                objs.isOneRoad = false;
            }

            //时间戳打找到同一帧的时间
            objs.timestamp = server->watchDatasCurTimestamp;

            if (server->queueObjs.push(objs)) {
                Info("待融合数据存入:选取的时间戳:%f", objs.timestamp);
            } else {
                Error("队列已满，未存入数据 timestamp:%f", objs.timestamp);
            }
        }
    }
}

void FusionServer::ThreadMerge(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

        //开始融合数据
        OBJS objs;
        if (!server->queueObjs.pop(objs)) {
            continue;
        }
//        Info("开始融合");
//        Info("待融合数据选取的时间戳:%lu", (uint64_t) objs.timestamp);

        //如果只有一路数据，不做多路融合，直接给出
        if (objs.isOneRoad) {
//            Info("只有一路数据，不走融合");
            MergeData mergeData;
            mergeData.timestamp = objs.timestamp;
            mergeData.obj.clear();

            Info("hasDataRoadIndex:%d", objs.hasDataRoadIndex);
            auto copyFrom = objs.roadData[objs.hasDataRoadIndex];

            //结果直接存输入
            for (auto iter:copyFrom.listObjs) {
                OBJECT_INFO_NEW item;
                OBJECT_INFO_T2OBJECT_INFO_NEW(iter, item);
                mergeData.obj.push_back(item);
            }

            mergeData.objInput.roadData[objs.hasDataRoadIndex].hardCode = copyFrom.hardCode;
            mergeData.objInput.roadData[objs.hasDataRoadIndex].imageData = copyFrom.imageData;
            mergeData.objInput.roadData[objs.hasDataRoadIndex].listObjs.assign(copyFrom.listObjs.begin(),
                                                                               copyFrom.listObjs.end());

            if (server->queueMergeData.push(mergeData)) {
//                Info("存入融合数据，size:%d", mergeData.obj.size());
            } else {
//                Error("队列已满，抛弃融合数据 ");
            }

            continue;
        }

        vector<OBJECT_INFO_NEW> vectorOBJNEW;
        OBJECT_INFO_NEW dataOut[1000];
        memset(dataOut, 0, ARRAY_SIZE(dataOut) * sizeof(OBJECT_INFO_NEW));

        int inOBJS = 0;

        for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {
            inOBJS += objs.roadData[i].listObjs.size();
        }

//        Info("传入目标数:%d", inOBJS);

        //存输入数据到文件
        if (0) {
            string fileName = to_string(server->frameCount) + "_in.txt";
            ofstream inDataFile;
            string inDataFileName = "mergeData/" + fileName;
            inDataFile.open(inDataFileName);
            string content;
            for (int j = 0; j < ARRAY_SIZE(objs.roadData); j++) {
                string split = ",";

                for (int m = 0; m < objs.roadData[j].listObjs.size(); m++) {
                    auto iter = objs.roadData[j].listObjs.at(m);
                    content += (to_string(j) + split +
                                to_string(iter.locationX) + split +
                                to_string(iter.locationY) + split +
                                to_string(iter.speedX) + split +
                                to_string(iter.speedY) + split +
                                to_string(iter.objID) + split +
                                to_string(iter.objType));
                    content += "\n";
                }
                //存入图片
                string inDataPicFileName =
                        "mergeData/" + to_string(server->frameCount) + "_in_" + to_string(j) + ".jpeg";
                ofstream inDataPicFile;
                inDataPicFile.open(inDataPicFileName);
                if (inDataPicFile.is_open()) {
                    inDataPicFile.write(objs.roadData[j].imageData.data(), objs.roadData[j].imageData.size());
                    Info("融合数据图片写入");
                    inDataPicFile.flush();
                    inDataPicFile.close();
                }
            }
            if (inDataFile.is_open()) {
                inDataFile.write((const char *) content.c_str(), content.size());
                Info("融合数据写入");
                inDataFile.flush();
                inDataFile.close();
            } else {
                Error("打开文件失败:%s", inDataFileName.c_str());
            }
        }

        int num = 0;
        switch (server->frame) {
            case 1: {
                //第一帧
                server->l1_obj.clear();
                server->l2_obj.clear();

                num = merge_total(server->repateX, server->widthX, server->widthY, server->Xmax, server->Ymax,
                                  server->gatetx, server->gatety, server->gatex, server->gatey, server->time_flag,
                                  objs.roadData[0].listObjs.data(), objs.roadData[0].listObjs.size(),
                                  objs.roadData[1].listObjs.data(), objs.roadData[1].listObjs.size(),
                                  objs.roadData[2].listObjs.data(), objs.roadData[2].listObjs.size(),
                                  objs.roadData[3].listObjs.data(), objs.roadData[3].listObjs.size(),
                                  server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                  server->l2_obj.size(), dataOut, server->angle_value);

                //斜路口
                /*     num = merge_total(server->flag_view, server->left_down_x, server->left_down_y,
                                       server->left_up_x, server->left_up_y,
                                       server->right_up_x, server->right_up_y,
                                       server->right_down_x, server->right_down_y,
                                       server->repateX, server->repateY,
                                       server->gatetx, server->gatety, server->gatex, server->gatey, server->time_flag,
                                       objs.roadData[0].listObjs.data(), objs.roadData[0].listObjs.size(),
                                       objs.roadData[1].listObjs.data(), objs.roadData[1].listObjs.size(),
                                       objs.roadData[2].listObjs.data(), objs.roadData[2].listObjs.size(),
                                       objs.roadData[3].listObjs.data(), objs.roadData[3].listObjs.size(),
                                       server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                       server->l2_obj.size(), dataOut, server->angle_value);
     */

                server->time_flag = false;

                //传递历史数据
                server->l2_obj.assign(server->l1_obj.begin(), server->l1_obj.end());

                server->l1_obj.clear();
                for (int i = 0; i < num; i++) {
                    server->l1_obj.push_back(dataOut[i]);
                }
                server->frame++;
                server->frameCount++;

                MergeData mergeData;
                mergeData.obj.clear();
                for (int i = 0; i < num; i++) {
                    mergeData.obj.push_back(dataOut[i]);
                }

                mergeData.timestamp = objs.timestamp;
                //存输入量
                for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {
                    auto copyFrom = objs.roadData[i];
                    mergeData.objInput.roadData[i].hardCode = copyFrom.hardCode;
                    mergeData.objInput.roadData[i].imageData = copyFrom.imageData;
                    if (!copyFrom.listObjs.empty()) {
                        mergeData.objInput.roadData[i].listObjs.assign(copyFrom.listObjs.begin(),
                                                                       copyFrom.listObjs.end());
                    }
                }

                if (server->queueMergeData.push(mergeData)) {
//                Info("存入融合数据，size:%d", mergeData.obj.size());
                } else {
//                Error("队列已满，抛弃融合数据 ");
                }

                //存输出数据到文件
                if (0) {
                    string fileNameO = to_string(server->frameCount - 1) + "_out.txt";
                    ofstream inDataFileO;
                    string inDataFileNameO = "mergeData/" + fileNameO;
                    inDataFileO.open(inDataFileNameO);
                    string contentO;
                    for (int j = 0; j < mergeData.obj.size(); j++) {
                        string split = ",";

                        auto iter = mergeData.obj.at(j);
                        contentO += (to_string(iter.objID1) + split +
                                     to_string(iter.objID2) + split +
                                     to_string(iter.objID3) + split +
                                     to_string(iter.objID4) + split +
                                     to_string(iter.showID) + split +
                                     to_string(iter.cameraID1) + split +
                                     to_string(iter.cameraID2) + split +
                                     to_string(iter.cameraID3) + split +
                                     to_string(iter.cameraID4) + split +
                                     to_string(iter.locationX) + split +
                                     to_string(iter.locationY) + split +
                                     to_string(iter.speed) + split +
                                     to_string(iter.flag_new));
                        contentO += "\n";
                    }
                    if (inDataFileO.is_open()) {
                        inDataFileO.write((const char *) contentO.c_str(), contentO.size());
//                        Info("融合数据写入");
                        inDataFileO.flush();
                        inDataFileO.close();
                    } else {
//                        Error("打开文件失败:%s", inDataFileNameO.c_str());
                    }
                }
            }
                break;
            default: {

                num = merge_total(server->repateX, server->widthX, server->widthY, server->Xmax, server->Ymax,
                                  server->gatetx, server->gatety, server->gatex, server->gatey, server->time_flag,
                                  objs.roadData[0].listObjs.data(), objs.roadData[0].listObjs.size(),
                                  objs.roadData[1].listObjs.data(), objs.roadData[1].listObjs.size(),
                                  objs.roadData[2].listObjs.data(), objs.roadData[2].listObjs.size(),
                                  objs.roadData[3].listObjs.data(), objs.roadData[3].listObjs.size(),
                                  server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                  server->l2_obj.size(), dataOut, server->angle_value);

                //斜路口
                /*    num = merge_total(server->flag_view, server->left_down_x, server->left_down_y,
                                      server->left_up_x, server->left_up_y,
                                      server->right_up_x, server->right_up_y,
                                      server->right_down_x, server->right_down_y,
                                      server->repateX, server->repateY,
                                      server->gatetx, server->gatety, server->gatex, server->gatey, server->time_flag,
                                      objs.roadData[0].listObjs.data(), objs.roadData[0].listObjs.size(),
                                      objs.roadData[1].listObjs.data(), objs.roadData[1].listObjs.size(),
                                      objs.roadData[2].listObjs.data(), objs.roadData[2].listObjs.size(),
                                      objs.roadData[3].listObjs.data(), objs.roadData[3].listObjs.size(),
                                      server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                      server->l2_obj.size(), dataOut, server->angle_value);
    */
                //传递历史数据
                server->l2_obj.assign(server->l1_obj.begin(), server->l1_obj.end());
                server->l1_obj.clear();
                for (int i = 0; i < num; i++) {
                    server->l1_obj.push_back(dataOut[i]);
                }

                server->frame++;

                if (server->frame > 2) {
                    server->frame = 2;
                }

                server->frameCount++;
                if (server->frameCount >= 0xffffffff) {
                    Info("帧计数满，从0开始");
                    server->frameCount = 0;
                }

                MergeData mergeData;
                mergeData.obj.clear();
                for (int i = 0; i < num; i++) {
                    mergeData.obj.push_back(dataOut[i]);
                }

                mergeData.timestamp = objs.timestamp;
                //存输入量
                for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {
                    auto copyFrom = objs.roadData[i];
                    mergeData.objInput.roadData[i].hardCode = copyFrom.hardCode;
                    mergeData.objInput.roadData[i].imageData = copyFrom.imageData;
                    if (!copyFrom.listObjs.empty()) {
                        mergeData.objInput.roadData[i].listObjs.assign(copyFrom.listObjs.begin(),
                                                                       copyFrom.listObjs.end());
                    }
                }

                if (server->queueMergeData.push(mergeData)) {
//                Info("存入融合数据，size:%d", mergeData.obj.size());
                } else {
//                Error("队列已满，抛弃融合数据 ");
                }

                //存输出数据到文件
                if (0) {
                    string fileNameO = to_string(server->frameCount - 1) + "_out.txt";
                    ofstream inDataFileO;
                    string inDataFileNameO = "mergeData/" + fileNameO;
                    inDataFileO.open(inDataFileNameO);
                    string contentO;
                    for (int j = 0; j < mergeData.obj.size(); j++) {
                        string split = ",";

                        auto iter = mergeData.obj.at(j);
                        contentO += (to_string(iter.objID1) + split +
                                     to_string(iter.objID2) + split +
                                     to_string(iter.objID3) + split +
                                     to_string(iter.objID4) + split +
                                     to_string(iter.showID) + split +
                                     to_string(iter.cameraID1) + split +
                                     to_string(iter.cameraID2) + split +
                                     to_string(iter.cameraID3) + split +
                                     to_string(iter.cameraID4) + split +
                                     to_string(iter.locationX) + split +
                                     to_string(iter.locationY) + split +
                                     to_string(iter.speed) + split +
                                     to_string(iter.flag_new));
                        contentO += "\n";
                    }
                    if (inDataFileO.is_open()) {
                        inDataFileO.write((const char *) contentO.c_str(), contentO.size());
                        inDataFileO.flush();
                        inDataFileO.close();
                    } else {
//                        Error("打开文件失败:%s", inDataFileNameO.c_str());
                    }
                }
            }

                break;
        }

    }
    Info("%s exit", __FUNCTION__);
}

void FusionServer::ThreadNotMerge(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

//        Info("不走融合直接发送数据");
        OBJS objs;
        if (!server->queueObjs.pop(objs)) {
            continue;
        }

//        Info("待融合数据存入:选取的时间戳:%f", objs.timestamp);

        MergeData mergeData;
        mergeData.obj.clear();

        const int INF = 0x7FFFFFFF;
        //北、东、南、西
        for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {

            auto iter = objs.roadData[i];

            for (int j = 0; j < iter.listObjs.size(); j++) {
                auto iter1 = iter.listObjs.at(j);
                OBJECT_INFO_NEW item;
                switch (i) {
                    case 0://North
                        item.objID1 = iter1.objID;
                        item.cameraID1 = iter1.cameraID;

                        item.objID2 = -INF;
                        item.cameraID2 = -INF;
                        item.objID3 = -INF;
                        item.cameraID3 = -INF;
                        item.objID4 = -INF;
                        item.cameraID4 = -INF;

                        item.showID = iter1.objID + 10000;
                        break;
                    case 1://East
                        item.objID2 = iter1.objID;
                        item.cameraID2 = iter1.cameraID;

                        item.objID1 = -INF;
                        item.cameraID1 = -INF;
                        item.objID3 = -INF;
                        item.cameraID3 = -INF;
                        item.objID4 = -INF;
                        item.cameraID4 = -INF;

                        item.showID = iter1.objID + 20000;
                        break;
                    case 2://South
                        item.objID3 = iter1.objID;
                        item.cameraID3 = iter1.cameraID;

                        item.objID2 = -INF;
                        item.cameraID2 = -INF;
                        item.objID1 = -INF;
                        item.cameraID1 = -INF;
                        item.objID4 = -INF;
                        item.cameraID4 = -INF;

                        item.showID = iter1.objID + 30000;
                        break;
                    case 3:
                        item.objID4 = iter1.objID;
                        item.cameraID4 = iter1.cameraID;

                        item.objID2 = -INF;
                        item.cameraID2 = -INF;
                        item.objID3 = -INF;
                        item.cameraID3 = -INF;
                        item.objID1 = -INF;
                        item.cameraID1 = -INF;

                        item.showID = iter1.objID + 40000;
                        break;
                }

                item.objType = iter1.objType;
                memcpy(item.plate_number, iter1.plate_number, sizeof(iter1.plate_number));
                memcpy(item.plate_color, iter1.plate_color, sizeof(iter1.plate_color));
                item.left = iter1.left;
                item.top = iter1.top;
                item.right = iter1.right;
                item.bottom = iter1.bottom;
                item.locationX = iter1.locationX;
                item.locationY = iter1.locationY;
                memcpy(item.distance, iter1.distance, sizeof(iter1.distance));
                item.directionAngle = iter1.directionAngle;
                item.speed = sqrt(iter1.speedX * iter1.speedX + iter1.speedY * iter1.speedY);
                item.latitude = iter1.latitude;
                item.longitude = iter1.longitude;
                item.flag_new = 0;

                mergeData.obj.push_back(item);
            }
        }


        mergeData.timestamp = objs.timestamp;
        //存输入量
        for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {
            auto copyFrom = objs.roadData[i];
            mergeData.objInput.roadData[i].hardCode = copyFrom.hardCode;
            mergeData.objInput.roadData[i].imageData = copyFrom.imageData;
            if (!copyFrom.listObjs.empty()) {
                mergeData.objInput.roadData[i].listObjs.assign(copyFrom.listObjs.begin(),
                                                               copyFrom.listObjs.end());
            }
        }


        if (server->queueMergeData.push(mergeData)) {
//                Info("存入融合数据，size:%d", mergeData.obj.size());
        } else {
//                Error("队列已满，抛弃融合数据 ");
        }

    }
    Info("%s exit", __FUNCTION__);
}

//寻找帧队列头部
void FusionServer::TaskFindOneFrame_TrafficFlow(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = server->dataUnit_TrafficFlows;
    if (server->isRun.load()) {

        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex = 0;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);

            //得到最大帧数
            if (iter.size() > maxPkgs) {
                maxPkgs = iter.size();
                maxPkgsIndex = i;
            }
        }
        vector<FlowData> flowDatas;//数据集合暂存
        if (maxPkgs > cache) {
            dataUnit.oneFrame.clear();
            //2.确定时间戳，取最大帧数的头部帧时间戳
            TrafficFlow refer;
            if (dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer)) {
                dataUnit.curTimestamp = refer.timstamp;
            }
            //            Info("TrafficFlows这次选取的时间戳标准为%lu", dataUnit.curTimestamp);
            for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
                auto iter = dataUnit.i_queue_vector.at(i);
                if (iter.empty()) {
//                  Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        TrafficFlow refer;
                        if (iter.front(refer)) {
                            if ((refer.timstamp >= (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame)) &&
                                (refer.timstamp <= (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
                                //出队列
                                TrafficFlow cur;
                                if (iter.pop(cur)) {
                                    //按路记录时间戳
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                    //TODO 赋值路口编号
                                    if (dataUnit.crossID.empty()) {
                                        dataUnit.crossID = cur.crossCode;
                                    }
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);

                                    for (auto item: cur.flowData) {
                                        flowDatas.push_back(item);
                                    }
                                    isFind = true;
                                }
                            } else if ((refer.timstamp < (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                TrafficFlow cur;
                                iter.pop(cur);
                            } else if ((refer.timstamp > (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }
                        }

                        if (iter.empty()) {
                            //队列为空，直接退出
                            isFind = true;
                        }
                    } while (!isFind);
                }
            }

            //组织数据放入
            TrafficFlows trafficFlows;
            trafficFlows.oprNum = random_uuid();
            trafficFlows.timestamp = dataUnit.curTimestamp;
            trafficFlows.crossID = dataUnit.crossID;

            trafficFlows.trafficFlow.assign(flowDatas.begin(), flowDatas.end());

            if (!dataUnit.o_queue.push(trafficFlows)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
            } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
            }
        }
    }
}


//寻找头部
void FusionServer::TaskFindOneFrame_LineupInfo(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = server->dataUnit_LineupInfoGather;
    if (server->isRun.load()) {

        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex = 0;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);

            //得到最大帧数
            if (iter.size() > maxPkgs) {
                maxPkgs = iter.size();
                maxPkgsIndex = i;
            }
        }
        vector<TrafficFlowLineup> TrafficFlowList;//数据集合暂存
        if (maxPkgs > cache) {
            dataUnit.oneFrame.clear();
            //2.确定时间戳，取最大帧数的头部帧时间戳
            LineupInfo refer;
            if (dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer)) {
                dataUnit.curTimestamp = refer.Timstamp;
            }
            //            Info("TrafficFlows这次选取的时间戳标准为%lu", dataUnit.curTimestamp);
            for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
                auto iter = dataUnit.i_queue_vector.at(i);
                if (iter.empty()) {
//                  Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        LineupInfo refer;
                        if (iter.front(refer)) {
                            if ((refer.Timstamp >= (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame)) &&
                                (refer.Timstamp <= (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
                                //出队列
                                LineupInfo cur;
                                if (iter.pop(cur)) {
                                    //按路记录时间戳
                                    dataUnit.xRoadTimestamp[i] = cur.Timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                    //TODO 赋值路口编号
                                    if (dataUnit.crossID.empty()) {
                                        dataUnit.crossID = cur.CrossCode;
                                    }
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);

                                    for (auto item: cur.TrafficFlowList) {
                                        TrafficFlowList.push_back(item);
                                    }
                                    isFind = true;
                                }
                            } else if ((refer.Timstamp < (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                LineupInfo cur;
                                iter.pop(cur);
                            } else if ((refer.Timstamp > (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }
                        }

                        if (iter.empty()) {
                            //队列为空，直接退出
                            isFind = true;
                        }
                    } while (!isFind);
                }
            }

            //组织数据放入
            LineupInfoGather lineupInfoGather;
            lineupInfoGather.OprNum = random_uuid();
            lineupInfoGather.Timstamp = dataUnit.curTimestamp;
            lineupInfoGather.CrossCode = dataUnit.crossID;

            lineupInfoGather.TrafficFlowList.assign(TrafficFlowList.begin(), TrafficFlowList.end());

            if (!dataUnit.o_queue.push(lineupInfoGather)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
            } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
            }
        }
    }
}

//寻找尾部
void FusionServer::TaskFindOneFrame_CrossTrafficJamAlarm(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = server->dataUnit_CrossTrafficJamAlarm;
    if (server->isRun.load()) {

        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex = 0;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);

            //得到最大帧数
            if (iter.size() > maxPkgs) {
                maxPkgs = iter.size();
                maxPkgsIndex = i;
            }
        }


        if (maxPkgs > cache) {
            dataUnit.oneFrame.clear();
            //2.确定时间戳，取最大帧数的尾部帧时间戳
            CrossTrafficJamAlarm refer;
            if (dataUnit.i_queue_vector.at(maxPkgsIndex).back(refer)) {
                dataUnit.curTimestamp = refer.Timstamp;
            }
            //            Info("TrafficFlows这次选取的时间戳标准为%lu", dataUnit.curTimestamp);
            for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
                auto iter = dataUnit.i_queue_vector.at(i);
                if (iter.empty()) {
//                  Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        CrossTrafficJamAlarm refer;
                        if (iter.front(refer)) {
                            if ((refer.Timstamp >= (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame)) &&
                                (refer.Timstamp <= (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
                                //出队列
                                CrossTrafficJamAlarm cur;
                                if (iter.pop(cur)) {
                                    //按路记录时间戳
                                    dataUnit.xRoadTimestamp[i] = cur.Timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                    //TODO 赋值路口编号
                                    if (dataUnit.crossID.empty()) {
                                        dataUnit.crossID = cur.CrossCode;
                                    }
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
                                    isFind = true;
                                }
                            } else if ((refer.Timstamp < (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                CrossTrafficJamAlarm cur;
                                iter.pop(cur);
                            } else if ((refer.Timstamp > (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }
                        }

                        if (iter.empty()) {
                            //队列为空，直接退出
                            isFind = true;
                        }
                    } while (!isFind);
                }
            }

            //组织数据放入
            CrossTrafficJamAlarm crossTrafficJamAlarm;
            crossTrafficJamAlarm.OprNum = random_uuid();
            crossTrafficJamAlarm.Timstamp = dataUnit.curTimestamp;
            crossTrafficJamAlarm.CrossCode = dataUnit.crossID;
            crossTrafficJamAlarm.AlarmType = 0;
            crossTrafficJamAlarm.AlarmStatus = 0;
            auto tp = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(tp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
            crossTrafficJamAlarm.AlarmTime = ss.str();
            //判断是否有拥堵
            for (auto iter:dataUnit.oneFrame) {
                if (iter.AlarmType == 1) {
                    crossTrafficJamAlarm.AlarmType = iter.AlarmType;
                    crossTrafficJamAlarm.AlarmStatus = iter.AlarmStatus;
                    crossTrafficJamAlarm.AlarmTime = iter.AlarmTime;
                }
            }

            if (!dataUnit.o_queue.push(crossTrafficJamAlarm)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
            } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
            }
        }
    }
}

void FusionServer::TaskFindOneFrame_MultiViewCarTracks(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = server->dataUnit_MultiViewCarTracks;
    if (server->isRun.load()) {

        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex = 0;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);

            //得到最大帧数
            if (iter.size() > maxPkgs) {
                maxPkgs = iter.size();
                maxPkgsIndex = i;
            }
        }
        vector<CarTrack> lstObj;//数据集合暂存
        if (maxPkgs > cache) {
            dataUnit.oneFrame.clear();
            //2.确定时间戳，取最大帧数的头部帧时间戳
            MultiViewCarTrack refer;
            if (dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer)) {
                dataUnit.curTimestamp = refer.timstamp;
            }
            //            Info("TrafficFlows这次选取的时间戳标准为%lu", dataUnit.curTimestamp);
            for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
                auto iter = dataUnit.i_queue_vector.at(i);
                if (iter.empty()) {
//                  Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        MultiViewCarTrack refer;
                        if (iter.front(refer)) {
                            if ((refer.timstamp >= (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame)) &&
                                (refer.timstamp <= (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
                                //出队列
                                MultiViewCarTrack cur;
                                if (iter.pop(cur)) {
                                    //按路记录时间戳
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                    //TODO 赋值路口编号
                                    if (dataUnit.crossID.empty()) {
                                        dataUnit.crossID = cur.crossCode;
                                    }
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);

                                    for (auto item: cur.lstObj) {
                                        lstObj.push_back(item);
                                    }
                                    isFind = true;
                                }
                            } else if ((refer.timstamp < (double) (dataUnit.curTimestamp - dataUnit.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                MultiViewCarTrack cur;
                                iter.pop(cur);
                            } else if ((refer.timstamp > (double) (dataUnit.curTimestamp + dataUnit.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }
                        }

                        if (iter.empty()) {
                            //队列为空，直接退出
                            isFind = true;
                        }
                    } while (!isFind);
                }
            }

            //组织数据放入
            MultiViewCarTracks multiViewCarTracks;
            multiViewCarTracks.oprNum = random_uuid();
            multiViewCarTracks.timestamp = dataUnit.curTimestamp;
            multiViewCarTracks.crossID = dataUnit.crossID;
            auto tp = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(tp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
            multiViewCarTracks.recordDateTime = ss.str();

            multiViewCarTracks.lstObj.assign(lstObj.begin(), lstObj.end());

            if (!dataUnit.o_queue.push(multiViewCarTracks)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
            } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
            }
        }
    }
}
