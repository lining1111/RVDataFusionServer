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
#include "multiView/MultiViewServer.h"
#include "merge/merge.h"
#include "merge/mergeStruct.h"
#include "simpleini/SimpleIni.h"
#include "sqlite3.h"


using namespace z_log;

MultiViewServer::MultiViewServer() {
    this->isRun.store(false);
    queueObjs.setMax(maxQueueObjs);
}

MultiViewServer::MultiViewServer(uint16_t port, string config, int maxListen) {
    this->port = port;
    this->config = config;
    this->maxListen = maxListen;
    this->isRun.store(false);
    queueObjs.setMax(maxQueueObjs);
}

MultiViewServer::~MultiViewServer() {
    Close();
}

void MultiViewServer::initConfig() {

    if (!config.empty()) {
        FILE *fp = fopen(config.c_str(), "r+");
        if (fp == nullptr) {
            Error("can not open file:%s", config.c_str());
            return;
        }
        CSimpleIniA ini;
        ini.LoadFile(fp);

//        const char *S_repateX = ini.GetValue("server", "repateX", "");
//        repateX = atof(S_repateX);
////        const char *S_widthX = ini.GetValue("server", "widthX", "");
////        widthX = atof(S_widthX);
////        const char *S_widthY = ini.GetValue("server", "widthY", "");
////        widthY = atof(S_widthY);
////        const char *S_Xmax = ini.GetValue("server", "Xmax", "");
////        Xmax = atof(S_Xmax);
////        const char *S_Ymax = ini.GetValue("server", "Ymax", "");
////        Ymax = atof(S_Ymax);
//        const char *S_gatetx = ini.GetValue("server", "gatetx", "");
//        gatetx = atof(S_gatetx);
//        const char *S_gatety = ini.GetValue("server", "gatety", "");
//        gatety = atof(S_gatety);
//        const char *S_gatex = ini.GetValue("server", "gatex", "");
//        gatex = atof(S_gatex);
//        const char *S_gatey = ini.GetValue("server", "gatey", "");
//        gatey = atof(S_gatey);

        fclose(fp);
    }
}


static int CallbackGetCL_ParkingArea(void *data, int argc, char **argv, char **azColName) {
    string colName;
    if (data != nullptr) {
        auto server = (MultiViewServer *) data;
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


void MultiViewServer::getMatrixNoFromDb() {
    sqlite3 *db;
    char *errmsg = nullptr;

    string dbName;
#ifdef arm64
    dbName = "/home/nvidianx/bin/" + this->db;
#else
    dbName = this->db;
#endif


    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
    }

    //base
    char *sqlGetCL_ParkingArea = "select * from CL_ParkingArea";
    rc = sqlite3_exec(db, sqlGetCL_ParkingArea, CallbackGetCL_ParkingArea, this, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
    }

}

static int CallbackGetIntersectionEntity(void *data, int argc, char **argv, char **azColName) {
    string colName;
    if (data != nullptr) {
        auto server = (MultiViewServer *) data;
        for (int i = 0; i < argc; i++) {
            colName = string(azColName[i]);
            if (colName.compare("plateId") == 0) {
                server->crossID = string(argv[i]);
                cout << "get crossID from db:" + server->crossID << endl;
            }
        }
    }

    return 0;
}

void MultiViewServer::getCrossIdFromDb() {
    sqlite3 *db;
    char *errmsg = nullptr;

    string dbName;
#ifdef arm64
    dbName = "/home/nvidianx/bin/" + this->eocDB;
#else
    dbName = this->eocDB;
#endif


    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
    }

    //intersectionEntity
    char *sqlGetIntersectionEntity = "select * from intersectionEntity";
    rc = sqlite3_exec(db, sqlGetIntersectionEntity, CallbackGetIntersectionEntity, this, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
    }
}


int MultiViewServer::Open() {
    //1.申请sock
    sock = socket(AF_INET, SOCK_STREAM/* | SOCK_NONBLOCK*/, IPPROTO_TCP);
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
        Fatal("multiView server sock bind error:%s", strerror(errno));
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
        Error("multiView server sock listen fail,error:%s", strerror(errno));
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

    Info("multiView server sock %d create success", sock);

    isRun.store(true);
    return 0;
}

int MultiViewServer::Run() {

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
    getCrossIdFromDb();

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

    //开启服务器多路数据融合线程
//    threadMerge = thread(ThreadMerge, this);
//    pthread_setname_np(threadMerge.native_handle(), "FusionServer merge");
//    threadMerge.detach();

//    //开启服务器多路数据不融合线程
//    threadNotMerge = thread(ThreadNotMerge, this);
//    pthread_setname_np(threadNotMerge.native_handle(), "FusionServer notMerge");
//    threadNotMerge.detach();

    return 0;
}

int MultiViewServer::Close() {
    if (sock > 0) {
        close(sock);
        sock = 0;
    }
    DeleteAllClient();

    isRun.store(false);

    return 0;
}

int MultiViewServer::setNonblock(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int MultiViewServer::AddClient(int client_sock, struct sockaddr_in remote_addr) {

    //多线程处理，不应该设置非阻塞模式
//    setNonblock(client_sock);
    // epoll add
    this->ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLRDHUP;
    this->ev.data.fd = client_sock;
    if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, client_sock, &this->ev) == -1) {
        Error("epoll add:%d fail,%s", client_sock, strerror(errno));
        return -1;
    }

    //clientInfo
    MultiViewClientInfo *clientInfo = new MultiViewClientInfo(remote_addr, client_sock);

    //add vector
    pthread_mutex_lock(&this->lockVectorClient);
    //根据ip，看下是否原来有记录，有的话，清除后在添加
    string addIp = string(inet_ntoa(clientInfo->clientAddr.sin_addr));
    for (int i = 0; i < this->vectorClient.size(); i++) {
        auto iter = this->vectorClient.at(i);
        string curIp = string(inet_ntoa(iter->clientAddr.sin_addr));
        if (curIp == addIp) {
            this->vectorClient.erase(this->vectorClient.begin() + i);
            this->vectorClient.shrink_to_fit();
            Info("remove repeat ip:%s", curIp.c_str());
        }
    }

    this->vectorClient.push_back(clientInfo);
    pthread_mutex_unlock(&this->lockVectorClient);

    Info("add client %d success,ip:%s", client_sock, addIp.c_str());

    //开启客户端处理线程
    clientInfo->ProcessRecv();

    return 0;
}

int MultiViewServer::RemoveClient(int client_sock) {

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
            break;
        }
    }
    pthread_mutex_unlock(&this->lockVectorClient);

    return 0;
}

int MultiViewServer::DeleteAllClient() {

    //删除客户端数组
    if (!vectorClient.empty()) {

        pthread_mutex_lock(&lockVectorClient);
        for (auto iter:vectorClient) {
            if (iter->sock > 0) {
                close(iter->sock);
                iter->sock = 0;
            }
            delete iter;
        }
        vectorClient.clear();
        pthread_mutex_unlock(&lockVectorClient);
    }

    return 0;
}

void MultiViewServer::ThreadMonitor(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (MultiViewServer *) pServer;

    int nfds = 0;// epoll监听事件发生的个数
    struct sockaddr_in remote_addr; // 客户端网络地址结构体
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int client_fd;

    Info("multiView %s run", __FUNCTION__);
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
                        iter->needRelease.store(true);
                        break;
                    }
                }
            }
        }
    }

    close(server->epoll_fd);
    close(server->sock);
    server->sock = 0;

    Info("multiView %s exit", __FUNCTION__);
}

void MultiViewServer::ThreadCheck(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (MultiViewServer *) pServer;

    Info("multiView %s run", __FUNCTION__);
    while (server->isRun.load()) {
        usleep(10);

        if (!server->isRun) {
            continue;
        }
        //检查客户端数组状态，
        // 主要是接收时间receive_time receive_time跟现在大于150秒则断开链接
        // needRelease上抛的释放标志，设置为真后，等待队列全部为空后进行释放
        if (server->vectorClient.empty()) {
            continue;
        }

        for (auto iter: server->vectorClient) {

            //检查超时
//            {
//                timeval tv;
//                gettimeofday(&tv, nullptr);
//                if ((tv.tv_sec - iter->receive_time.tv_sec) >= server->checkStatusTimeval.tv_usec) {
//                    iter->needRelease.store(true);
//                }
//
//            }
            //检查needRelease
            {
                if (iter->needRelease.load()) {

                    server->RemoveClient(iter->sock);
                }
            }
        }

    }

    Info("multiView %s exit", __FUNCTION__);

}

void MultiViewServer::ThreadFindOneFrame(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (MultiViewServer *) pServer;

    Info("multiView %s run", __FUNCTION__);
    while (server->isRun.load()) {

        sleep(1);

        if (!server->isRun) {
            continue;
        }

        //如果连接上的客户端数量为0
        if (server->vectorClient.size() == 0) {
//            Info("未连入客户端");
            continue;
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
            continue;
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
                server->curTimestamp = trafficFlow.timstamp;//一直取第一路的时间戳为基准
            }

//            Info("multiView这次选取的时间戳标准为%lu", server->curTimestamp);

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
                            TrafficFlow trafficFlow;
                            if (curClient->queueTrafficFlow.front(trafficFlow)) {
                                if ((trafficFlow.timstamp >=
                                     (double) (server->curTimestamp - server->thresholdFrame)) &&
                                    (trafficFlow.timstamp <=
                                     (double) (server->curTimestamp + server->thresholdFrame))) {
                                    //出队列
                                    curClient->queueTrafficFlow.pop(trafficFlow);
                                    //按路记录时间戳
                                    server->xRoadTimestamp[i] = trafficFlow.timstamp;
//                                Info("multiView第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                    //TODO 赋值路口编号
                                    if (server->crossID.empty()) {
                                        server->crossID = trafficFlow.crossCode;
                                    }
                                    //时间戳大于当前时间戳，直接取

                                    for (auto item: trafficFlow.flowData) {
                                        flowDatas.push_back(item);
                                    }
                                    isFind = true;
                                } else if ((trafficFlow.timstamp <
                                            (double) (server->curTimestamp - server->thresholdFrame))) {
                                    //小于当前时间戳或者不在门限内，出队列抛弃
//                                Info("multiView第%d路时间戳%lu靠前,舍弃", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                    //出队列
                                    curClient->queueTrafficFlow.pop(trafficFlow);

                                } else if ((trafficFlow.timstamp >
                                            (double) (server->curTimestamp + server->thresholdFrame))) {
//                                Info("multiView第%d路时间戳%lu靠后,保留", i + 1,
//                                     (uint64_t) curClient->queueTrafficFlow.front().timstamp);
                                    isFind = true;
                                }

                                if (curClient->queueTrafficFlow.empty()){
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
            trafficFlows.timestamp = server->curTimestamp;
            trafficFlows.crossID = server->crossID;

            trafficFlows.trafficFlow.assign(flowDatas.begin(), flowDatas.end());

            if (!server->queueObjs.push(trafficFlows)) {
//                Error("multiView队列已满，未存入数据 timestamp:%f", trafficFlows.timestamp);
            } else {
//                Info("multiView数据存入:选取的时间戳:%f", trafficFlows.timestamp);
            }

            maxPkgs--;
        pthread_mutex_unlock(&server->lockVectorClient);

    }

    Info("multiView %s exit", __FUNCTION__);
}

