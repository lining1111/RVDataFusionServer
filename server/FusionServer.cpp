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

    queueObjs.setMax(maxQueueObjs);
    queueMergeData.setMax(maxQueueMergeData);
}

FusionServer::FusionServer(uint16_t port, string config, int maxListen, bool isMerge) {
    this->port = port;
    this->config = config;
    this->maxListen = maxListen;
    this->isRun.store(false);
    this->isMerge = isMerge;

    queueObjs.setMax(maxQueueObjs);
    queueMergeData.setMax(maxQueueMergeData);
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
                server->crossID = string(argv[i]);
                cout << "get crossID from db:" + server->crossID << endl;
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

    //开启服务器检查客户端线程
//    threadCheck = thread(ThreadCheck, this);
//    pthread_setname_np(threadCheck.native_handle(), "FusionServer check");
//    threadCheck.detach();

    //开启服务器获取一帧大数据线程
    threadFindOneFrame = thread(ThreadFindOneFrame, this);
    pthread_setname_np(threadFindOneFrame.native_handle(), "FusionServer findOneFrame");
    threadFindOneFrame.detach();

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

    //
    threadFindOneFrame_TrafficFlow = thread(ThreadFindOneFrame_TrafficFlow, this);
    pthread_setname_np(threadFindOneFrame_TrafficFlow.native_handle(), "FusionServer findOneFrame_TrafficFlow");
    threadFindOneFrame_TrafficFlow.detach();

    threadFindOneFrame_LineupInfo = thread(ThreadFindOneFrame_LineupInfo, this);
    pthread_setname_np(threadFindOneFrame_LineupInfo.native_handle(), "FusionServer findOneFrame_LineupInfo");
    threadFindOneFrame_LineupInfo.detach();


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
    ClientInfo *clientInfo = new ClientInfo(remote_addr, client_sock);

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

void FusionServer::ThreadFindOneFrame(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

        usleep(1000 * 100);

        if (!server->isRun) {
            continue;
        }

        //如果连接上的客户端数量为0
        if (server->vectorClient.size() == 0) {
//            Info("未连入客户端");
            continue;
        }
        //所有的客户端都要缓存3帧数据
        int clientNum = server->vectorClient.size();//连入的客户端数量
        int maxPkgs = 0;//轮询各个路存有的最大帧数
        bool hasData = false;
        for (int i = 0; i < server->vectorClient.size(); i++) {
            auto iter = server->vectorClient.at(i);
//            Info("client-%d,watchData size:%d", iter->sock, iter->queueWatchData.size());
            //得到最大帧数
            if (iter->queueWatchData.size() > maxPkgs) {
                maxPkgs = iter->queueWatchData.size();
            }
        }
//        Info("clientNum %d,maxPkgs %d", clientNum, maxPkgs);
        if (maxPkgs >= 10) {
            hasData = true;
        }
        if (!hasData) {
//            Info("全部路数未有3帧数据缓存");
            continue;
        }

        pthread_mutex_lock(&server->lockVectorClient);
        //如果有三帧数据，一直取到就有3帧

//        Info("maxPkgs:%d", maxPkgs);
        Info("寻找同一帧数据");
        OBJS objs;//容量为4,依次是北、东、南、西

        //轮询各个连入的客户端，按指定的路口ip来存入数组

        int id[4] = {-1, -1, -1, -1};//以方向为目的存入的路口方向对应的客户端数组的索引
        int connetClientNum = 0;
        //1.寻找各个路口的客户端索引，未找到就是默认值-1
        for (int i = 0; i < server->vectorClient.size(); i++) {
            auto iter = server->vectorClient.at(i);
            for (int j = 0; j < server->roadDirection.size(); j++) {
                //ip 相同
                if (iter->direction == server->roadDirection.at(j)) {
                    id[j] = i;
//                        Info("第%d路 client数组索引%d sock：%d", j + 1, id[j], server->vector_client.at(i)->sock);
                    connetClientNum++;
                }
            }
        }

        //2.确定时间戳
        WatchData watchData;
        if (server->vectorClient.at(0)->queueWatchData.front(watchData)) {
            server->curTimestamp = watchData.timstamp;//一直取第一路的时间戳为基准
            Info("选取的第一路时间戳，方向:%d", int(server->vectorClient.at(0)->direction));
        } else {
            server->curTimestamp += 100;
            Info("时间戳自动加100");
        }

        //如果现在的时间戳比本地的系统时间差2分钟，则用本地时间-1分钟的标准
        struct timeval now;
        gettimeofday(&now, nullptr);
        uint64_t timestamp = (now.tv_sec * 1000 + now.tv_usec / 1000) - 1 * 60 * 1000;
        if (server->curTimestamp < timestamp) {
            Info("选取的系统时间-1分钟");
            server->curTimestamp = timestamp;
        }


        Info("这次选取的时间戳标准为%lu", server->curTimestamp);


        //2.根据时间戳和门限来赋值
        RoadData obj[4];

        for (int i = 0; i < ARRAY_SIZE(obj); i++) {
            obj[i].imageData = "";
            obj[i].hardCode = "";
            obj[i].listObjs.clear();
        }
        //清理第N路的取帧时间戳记录
        for (int i = 0; i < ARRAY_SIZE(server->xRoadTimestamp); i++) {
            server->xRoadTimestamp[i] = 0;
        }

        //测量本次内时间差用的临时变量
        uint64_t max = 0;
        uint64_t min = 0;

        //取数据
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] == -1) {
                //客户端未连入，不填充数据
            } else {
                //客户端连入，填充数据
                auto curClient = server->vectorClient.at(id[i]);
                //watchData数据
                if (curClient->queueWatchData.empty()) {
                    //客户端未有数据传入，不填充数据
                    Info("第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        WatchData watchData;
                        if (curClient->queueWatchData.front(watchData)) {
                            if ((watchData.timstamp >= (double) (server->curTimestamp - server->thresholdFrame)) &&
                                (watchData.timstamp <= (double) (server->curTimestamp + server->thresholdFrame))) {
                                //出队列
                                curClient->queueWatchData.pop(watchData);
                                //按路记录时间戳
                                server->xRoadTimestamp[i] = watchData.timstamp;
                                Info("第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);

                                //get max min
                                if (max == 0) {
                                    max = (uint64_t) watchData.timstamp;
                                } else {
                                    max = (uint64_t) watchData.timstamp > max ? (uint64_t) watchData.timstamp : max;
                                }

                                if (min == 0) {
                                    min = (uint64_t) watchData.timstamp;
                                } else {
                                    min = (uint64_t) watchData.timstamp < min ? (uint64_t) watchData.timstamp : min;
                                }


                                //TODO 赋值路口编号
                                if (server->crossID.empty()) {
                                    server->crossID = watchData.matrixNo;
                                }
                                //时间戳大于当前时间戳，直接取

                                obj[i].hardCode = watchData.hardCode;
                                obj[i].imageData = watchData.imageData;
//                                    Info("寻找同一帧，找到第%d路的数据后，hardCode:%s 图像大小%d", i, obj[i].hardCode.c_str(),
//                                         obj[i].imageData.size());
                                for (auto item: watchData.lstObjTarget) {
                                    OBJECT_INFO_T objectInfoT;
                                    //转换
                                    ObjTarget2OBJECT_INFO_T(item, objectInfoT);
                                    obj[i].listObjs.push_back(objectInfoT);
                                }
                                isFind = true;
                            } else if ((watchData.timstamp <
                                        (double) (server->curTimestamp - server->thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
                                Info("第%d路时间戳%lu靠前,舍弃", i + 1, (uint64_t) watchData.timstamp);
                                //出队列抛弃
                                curClient->queueWatchData.pop(watchData);

                            } else if ((watchData.timstamp >
                                        (double) (server->curTimestamp + server->thresholdFrame))) {
                                Info("第%d路时间戳%lu靠后,保留", i + 1, (uint64_t) watchData.timstamp);
                                isFind = true;
                            }

                            //如果全部抛完
                            if (curClient->queueWatchData.empty()) {
                                //队列为空，直接退出
                                Info("第%d路队列为空直接退出", i + 1);
                                isFind = true;
                            }

                        }
                    } while (!isFind);
                }
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
        if (connetClientNum == 1) {
            for (int i = 0; i < ARRAY_SIZE(obj); i++) {
                if (!obj[i].listObjs.empty() || !obj[i].imageData.empty()) {
                    objs.hasDataRoadIndex = i;
//                        Info("hasDataRoadIndex:%d", objs.hasDataRoadIndex);
                    break;
                }
            }
            objs.isOneRoad = true;
        } else {
            objs.isOneRoad = false;
        }

        for (int i = 0; i < ARRAY_SIZE(objs.roadData); i++) {
            objs.roadData[i].hardCode = obj[i].hardCode;
            objs.roadData[i].imageData = obj[i].imageData;
            objs.roadData[i].listObjs.assign(obj[i].listObjs.begin(), obj[i].listObjs.end());
        }

        //时间戳打找到同一帧的时间
        objs.timestamp = server->curTimestamp;

        if (server->queueObjs.push(objs)) {
            Info("待融合数据存入:选取的时间戳:%f", objs.timestamp);
        } else {
            Error("队列已满，未存入数据 timestamp:%f", objs.timestamp);
        }

        maxPkgs--;

        pthread_mutex_unlock(&server->lockVectorClient);

    }

    Info("%s exit", __FUNCTION__);
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

void FusionServer::TaskFindOneFrame_TrafficFlow(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    if (server->isRun.load()) {

        //如果连接上的客户端数量为0
        if (server->vectorClient.size() == 0) {
//            Info("未连入客户端");
            return;
        }

        int maxPkgs = 0;//轮询各个路存有的最大帧数
        bool hasData = false;
        for (int i = 0; i < server->vectorClient.size(); i++) {
            auto iter = server->vectorClient.at(i);

            //得到最大帧数
            if (iter->queueTrafficFlow.size() > maxPkgs) {
                maxPkgs = iter->queueTrafficFlow.size();
            }
        }
        if (maxPkgs >= 3) {
            hasData = true;
        }
        if (!hasData) {
//            Info("全部路数未有3帧数据缓存");
            return;
        }
        pthread_mutex_lock(&server->lockVectorClient);
        //如果有三帧数据，一直取到就有3帧
//            Info("multiView寻找同一帧数据");


        uint64_t timestamp = 0;//时间戳，选取时间最近的方向

        //轮询各个连入的客户端，按指定的路口ip来存入数组

        int id[4] = {-1, -1, -1, -1};//以方向为目的存入的路口方向对应的客户端数组的索引
        int connetClientNum = 0;

        for (int i = 0; i < server->vectorClient.size(); i++) {
            id[i] = i;
        }

        //2.确定时间戳
        TrafficFlow trafficFlow;
        if (server->vectorClient.at(0)->queueTrafficFlow.front(trafficFlow)) {
            server->dataUnit_TrafficFlows.curTimestamp = trafficFlow.timstamp;//一直取第一路的时间戳为基准
        }

//            Info("TrafficFlows这次选取的时间戳标准为%lu", server->curTimestamp);

        vector<FlowData> flowDatas;
        //取数据
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] == -1) {
                //客户端未连入，不填充数据
            } else {
                //客户端连入，填充数据
                auto curClient = server->vectorClient.at(id[i]);
                //watchData数据
                if (curClient->queueTrafficFlow.empty()) {
                    //客户端未有数据传入，不填充数据
//                        Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        TrafficFlow trafficFlow1;
                        if (curClient->queueTrafficFlow.front(trafficFlow1)) {
                            if ((trafficFlow1.timstamp >=
                                 (double) (server->dataUnit_TrafficFlows.curTimestamp -
                                           server->dataUnit_TrafficFlows.thresholdFrame)) &&
                                (trafficFlow1.timstamp <= (double) (server->dataUnit_TrafficFlows.curTimestamp +
                                                                    server->dataUnit_TrafficFlows.thresholdFrame))) {
                                //出队列
                                curClient->queueTrafficFlow.pop(trafficFlow1);
                                //按路记录时间戳
                                server->dataUnit_TrafficFlows.xRoadTimestamp[i] = trafficFlow1.timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                //TODO 赋值路口编号
                                if (server->dataUnit_TrafficFlows.crossID.empty()) {
                                    server->dataUnit_TrafficFlows.crossID = trafficFlow1.crossCode;
                                }
                                //时间戳大于当前时间戳，直接取

                                for (auto item: trafficFlow1.flowData) {
                                    flowDatas.push_back(item);
                                }
                                isFind = true;
                            } else if ((trafficFlow1.timstamp <
                                        (double) (server->dataUnit_TrafficFlows.curTimestamp -
                                                  server->dataUnit_TrafficFlows.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                curClient->queueTrafficFlow.pop(trafficFlow1);

                            } else if ((trafficFlow1.timstamp >
                                        (double) (server->dataUnit_TrafficFlows.curTimestamp +
                                                  server->dataUnit_TrafficFlows.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }

                            if (curClient->queueTrafficFlow.empty()) {
                                //队列为空，直接退出
                                isFind = true;
                            }
                        }
                    } while (!isFind);
                }
            }
        }

        TrafficFlows trafficFlows;
        trafficFlows.oprNum = random_uuid();
        trafficFlows.timestamp = server->dataUnit_TrafficFlows.curTimestamp;
        trafficFlows.crossID = server->dataUnit_TrafficFlows.crossID;

        trafficFlows.trafficFlow.assign(flowDatas.begin(), flowDatas.end());

        if (!server->dataUnit_TrafficFlows.m_queue.push(trafficFlows)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
        } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
        }

        maxPkgs--;
        pthread_mutex_unlock(&server->lockVectorClient);
    }
}

void FusionServer::ThreadFindOneFrame_TrafficFlow(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Timer timer;
    timer.start(1000, std::bind(TaskFindOneFrame_TrafficFlow, server));


    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

        sleep(1);
        timer.stop();
    }

    Info("%s exit", __FUNCTION__);
}


void FusionServer::TaskFindOneFrame_LineupInfo(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    if (server->isRun.load()) {
        //如果连接上的客户端数量为0
        if (server->vectorClient.size() == 0) {
//            Info("未连入客户端");
            return;
        }


        int maxPkgs = 0;//轮询各个路存有的最大帧数
        bool hasData = false;
        for (int i = 0; i < server->vectorClient.size(); i++) {
            auto iter = server->vectorClient.at(i);

            //得到最大帧数
            if (iter->queueLineupInfo.size() > maxPkgs) {
                maxPkgs = iter->queueLineupInfo.size();
            }
        }
        if (maxPkgs >= 3) {
            hasData = true;
        }
        if (!hasData) {
//            Info("全部路数未有3帧数据缓存");
            return;
        }
        pthread_mutex_lock(&server->lockVectorClient);
        //如果有三帧数据，一直取到就有3帧
//            Info("multiView寻找同一帧数据");

        uint64_t timestamp = 0;//时间戳，选取时间最近的方向

        //轮询各个连入的客户端，按指定的路口ip来存入数组

        int id[4] = {-1, -1, -1, -1};//以方向为目的存入的路口方向对应的客户端数组的索引
        int connetClientNum = 0;

        for (int i = 0; i < server->vectorClient.size(); i++) {
            id[i] = i;
        }

        //2.确定时间戳
        LineupInfo lineupInfo;
        if (server->vectorClient.at(0)->queueLineupInfo.front(lineupInfo)) {
            server->dataUnit_LineupInfoGather.curTimestamp = lineupInfo.Timstamp;//一直取第一路的时间戳为基准
        }

//            Info("multiView这次选取的时间戳标准为%lu", server->curTimestamp);

        vector<TrafficFlowLineup> TrafficFlowList;
        //取数据
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] == -1) {
                //客户端未连入，不填充数据
            } else {
                //客户端连入，填充数据
                auto curClient = server->vectorClient.at(id[i]);
                //watchData数据
                if (curClient->queueLineupInfo.empty()) {
                    //客户端未有数据传入，不填充数据
//                        Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        LineupInfo lineupInfo1;
                        if (curClient->queueLineupInfo.front(lineupInfo1)) {
                            if ((lineupInfo1.Timstamp >=
                                 (double) (server->dataUnit_LineupInfoGather.curTimestamp -
                                           server->dataUnit_LineupInfoGather.thresholdFrame)) &&
                                (lineupInfo1.Timstamp <=
                                 (double) (server->dataUnit_LineupInfoGather.curTimestamp +
                                           server->dataUnit_LineupInfoGather.thresholdFrame))) {
                                //出队列
                                curClient->queueLineupInfo.pop(lineupInfo1);
                                //按路记录时间戳
                                server->dataUnit_LineupInfoGather.xRoadTimestamp[i] = lineupInfo1.Timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                //TODO 赋值路口编号
                                if (server->dataUnit_LineupInfoGather.crossID.empty()) {
                                    server->dataUnit_LineupInfoGather.crossID = lineupInfo1.CrossCode;
                                }
                                //时间戳大于当前时间戳，直接取

                                for (auto item: lineupInfo1.TrafficFlowList) {
                                    TrafficFlowList.push_back(item);
                                }
                                isFind = true;
                            } else if ((lineupInfo1.Timstamp <
                                        (double) (server->dataUnit_LineupInfoGather.curTimestamp -
                                                  server->dataUnit_LineupInfoGather.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                curClient->queueLineupInfo.pop(lineupInfo1);

                            } else if ((lineupInfo1.Timstamp >
                                        (double) (server->dataUnit_LineupInfoGather.curTimestamp +
                                                  server->dataUnit_LineupInfoGather.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }

                            if (curClient->queueTrafficFlow.empty()) {
                                //队列为空，直接退出
                                isFind = true;
                            }
                        }
                    } while (!isFind);
                }
            }
        }

        LineupInfoGather lineupInfoGather;
        lineupInfoGather.OprNum = random_uuid();
        lineupInfoGather.Timstamp = server->dataUnit_LineupInfoGather.curTimestamp;
        lineupInfoGather.CrossCode = server->dataUnit_LineupInfoGather.crossID;
        lineupInfoGather.HardCode = server->matrixNo;
        lineupInfoGather.TrafficFlowList.assign(TrafficFlowList.begin(), TrafficFlowList.end());

        if (!server->dataUnit_LineupInfoGather.m_queue.push(lineupInfoGather)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
        } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
        }

        maxPkgs--;
        pthread_mutex_unlock(&server->lockVectorClient);

    }
}

void FusionServer::ThreadFindOneFrame_LineupInfo(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Timer timer;
    timer.start(1000, std::bind(TaskFindOneFrame_LineupInfo, server));


    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

        sleep(1);
        timer.stop();
    }

    Info("%s exit", __FUNCTION__);
}

void FusionServer::TaskFindOneFrame_CrossTrafficJamAlarm(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    if (server->isRun.load()) {
        //如果连接上的客户端数量为0
        if (server->vectorClient.size() == 0) {
//            Info("未连入客户端");
            return;
        }


        int maxPkgs = 0;//轮询各个路存有的最大帧数
        int maxPkgsIndex = 0;//最多帧sock数组索引
        bool hasData = false;
        for (int i = 0; i < server->vectorClient.size(); i++) {
            auto iter = server->vectorClient.at(i);

            //得到最大帧数
            if (iter->queueCrossTrafficJamAlarm.size() > maxPkgs) {
                maxPkgs = iter->queueCrossTrafficJamAlarm.size();
                maxPkgsIndex = i;
            }
        }
        if (maxPkgs >= 10) {
            hasData = true;
        }
        if (!hasData) {
//            Info("全部路数未有3帧数据缓存");
            return;
        }
        pthread_mutex_lock(&server->lockVectorClient);
        //如果有三帧数据，一直取到就有3帧
//            Info("multiView寻找同一帧数据");

        uint64_t timestamp = 0;//时间戳，选取时间最近的方向

        //轮询各个连入的客户端，按指定的路口ip来存入数组

        int id[4] = {-1, -1, -1, -1};//以方向为目的存入的路口方向对应的客户端数组的索引
        int connetClientNum = 0;

        for (int i = 0; i < server->vectorClient.size(); i++) {
            id[i] = i;
        }

        //2.确定时间戳
        CrossTrafficJamAlarm crossTrafficJamAlarm;

        //先抛1帧
        CrossTrafficJamAlarm foo;
        server->vectorClient.at(maxPkgsIndex)->queueCrossTrafficJamAlarm.pop(foo);
        if (server->vectorClient.at(maxPkgsIndex)->queueCrossTrafficJamAlarm.front(crossTrafficJamAlarm)) {
            server->dataUnit_CrossTrafficJamAlarm.curTimestamp = crossTrafficJamAlarm.Timstamp;//一直取第一路的时间戳为基准
        }

//            Info("multiView这次选取的时间戳标准为%lu", server->curTimestamp);

        vector<CrossTrafficJamAlarm> CrossTrafficJamAlarmList;
        //取数据
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] == -1) {
                //客户端未连入，不填充数据
            } else {
                //客户端连入，填充数据
                auto curClient = server->vectorClient.at(id[i]);
                //watchData数据
                if (curClient->queueCrossTrafficJamAlarm.empty()) {
                    //客户端未有数据传入，不填充数据
//                        Info("multiView第%d路未有数据传入，不填充数据", i + 1);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        CrossTrafficJamAlarm crossTrafficJamAlarm1;
                        if (curClient->queueCrossTrafficJamAlarm.front(crossTrafficJamAlarm1)) {
                            if ((crossTrafficJamAlarm1.Timstamp >=
                                 (double) (server->dataUnit_CrossTrafficJamAlarm.curTimestamp -
                                           server->dataUnit_CrossTrafficJamAlarm.thresholdFrame)) &&
                                (crossTrafficJamAlarm1.Timstamp <=
                                 (double) (server->dataUnit_CrossTrafficJamAlarm.curTimestamp +
                                           server->dataUnit_CrossTrafficJamAlarm.thresholdFrame))) {
                                //出队列
                                curClient->queueCrossTrafficJamAlarm.pop(crossTrafficJamAlarm1);
                                //按路记录时间戳
                                server->dataUnit_CrossTrafficJamAlarm.xRoadTimestamp[i] = crossTrafficJamAlarm1.Timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                //TODO 赋值路口编号
                                if (server->dataUnit_CrossTrafficJamAlarm.crossID.empty()) {
                                    server->dataUnit_CrossTrafficJamAlarm.crossID = crossTrafficJamAlarm1.CrossCode;
                                }
                                //时间戳大于当前时间戳，直接取
                                CrossTrafficJamAlarmList.push_back(crossTrafficJamAlarm1);
                                isFind = true;
                            } else if ((crossTrafficJamAlarm1.Timstamp <
                                        (double) (server->dataUnit_CrossTrafficJamAlarm.curTimestamp -
                                                  server->dataUnit_CrossTrafficJamAlarm.thresholdFrame))) {
                                //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                //出队列
                                curClient->queueCrossTrafficJamAlarm.pop(crossTrafficJamAlarm1);

                            } else if ((crossTrafficJamAlarm1.Timstamp >
                                        (double) (server->dataUnit_CrossTrafficJamAlarm.curTimestamp +
                                                  server->dataUnit_CrossTrafficJamAlarm.thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                isFind = true;
                            }

                            if (curClient->queueCrossTrafficJamAlarm.empty()) {
                                //队列为空，直接退出
                                isFind = true;
                            }
                        }
                    } while (!isFind);
                }
            }
        }

        bool isJam = false;
        string jamTime;
        for (auto iter:CrossTrafficJamAlarmList) {
            if (iter.AlarmType == 1) {
                isJam = true;
                jamTime = iter.AlarmTime;
            }
        }


        CrossTrafficJamAlarm crossTrafficJamAlarm2;
        crossTrafficJamAlarm2.OprNum = random_uuid();
        crossTrafficJamAlarm2.Timstamp = server->dataUnit_CrossTrafficJamAlarm.curTimestamp;
        crossTrafficJamAlarm2.CrossCode = server->dataUnit_CrossTrafficJamAlarm.crossID;
        crossTrafficJamAlarm2.HardCode = server->matrixNo;
        if (isJam) {
            crossTrafficJamAlarm2.AlarmType = 1;
            crossTrafficJamAlarm2.AlarmStatus = 1;
            crossTrafficJamAlarm2.AlarmTime = jamTime;
        } else {
            crossTrafficJamAlarm2.AlarmType = 0;
            crossTrafficJamAlarm2.AlarmStatus = 0;
            crossTrafficJamAlarm2.AlarmTime = jamTime;
        }

        if (!server->dataUnit_CrossTrafficJamAlarm.m_queue.push(crossTrafficJamAlarm2)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
        } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
        }

        maxPkgs--;
        pthread_mutex_unlock(&server->lockVectorClient);

    }
}

void FusionServer::ThreadFindOneFrame_CrossTrafficJamAlarm(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Timer timer;
    timer.start(1000, std::bind(TaskFindOneFrame_CrossTrafficJamAlarm, server));


    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {

        sleep(1);
        timer.stop();
    }

    Info("%s exit", __FUNCTION__);
}


