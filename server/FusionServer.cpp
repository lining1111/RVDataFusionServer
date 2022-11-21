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
#include <string>
#include <sstream>
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

    dataUnitFusionData.init(30, 100, 100, 4);//100ms一帧
    dataUnitMultiViewCarTracks.init(30, 66, 33, 4);//66ms一帧
    dataUnitTrafficFlows.init(30, 1000, 500, 4);//1000ms一帧
    dataUnitCrossTrafficJamAlarm.init(30, 1000, 500, 4);//1000ms一帧
    dataUnitLineupInfoGather.init(30, 1000, 500, 4);//1000ms一帧

}

FusionServer::FusionServer(uint16_t port, string config, int maxListen, bool isMerge) {
    this->port = port;
    this->config = config;
    this->maxListen = maxListen;
    this->isRun.store(false);
    this->isMerge = isMerge;
    dataUnitFusionData.init(30, 100, 100, 4);//100ms一帧
    dataUnitMultiViewCarTracks.init(30, 66, 33, 4);//66ms一帧
    dataUnitTrafficFlows.init(30, 1000, 500, 4);//1000ms一帧
    dataUnitCrossTrafficJamAlarm.init(30, 1000, 500, 4);//1000ms一帧
    dataUnitLineupInfoGather.init(30, 1000, 500, 4);//1000ms一帧
}

FusionServer::~FusionServer() {
    deleteTimerTaskAll();
    Close();
}

int FusionServer::getMatrixNoFromDb() {
    //打开数据库
    sqlite3 *db;
    string dbName;
#ifdef arm64
    dbName = "/home/nvidianx/bin/CLParking.db";
#else
    dbName = "./db/CLParking.db";
#endif
    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        printf("sqlite open fail\n");
        sqlite3_close(db);
        return -1;
    }

    char *sql = "select UName from CL_ParkingArea";
    char **result, *errmsg;
    int nrow, ncolumn, i, j, index;
    string columnName;
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        printf("sqlite err:%s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    for (int m = 0; m < nrow; m++) {
        for (int n = 0; n < ncolumn; n++) {
            columnName = string(result[n]);
            if (columnName == "UName") {
                matrixNo = STR(result[ncolumn + n + m * nrow]);
                break;
            }
        }
    }
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
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

    server->addTimerTask("timerFusionData", server->dataUnitFusionData.fs_ms,
                         std::bind(TaskFindOneFrame_FusionData, server, 10));

    server->addTimerTask("timerTrafficFlow", server->dataUnitTrafficFlows.fs_ms,
                         std::bind(TaskFindOneFrame_TrafficFlow, server, 10));
    server->addTimerTask("timerLineupInfoGather", server->dataUnitLineupInfoGather.fs_ms,
                         std::bind(TaskFindOneFrame_LineupInfoGather, server, 10));

    server->addTimerTask("timerCrossTrafficJamAlarm", server->dataUnitCrossTrafficJamAlarm.fs_ms,
                         std::bind(TaskFindOneFrame_CrossTrafficJamAlarm, server, 10));

    server->addTimerTask("timerMultiViewCarTracks", server->dataUnitMultiViewCarTracks.fs_ms,
                         std::bind(TaskFindOneFrame_MultiViewCarTracks, server, 10));

    Info("%s exit", __FUNCTION__);
}

void FusionServer::addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task) {
    Timer *timer = new Timer();
    timer->start(timeval_ms, task);
    timerTasks.insert(make_pair(name, timer));
}

void FusionServer::deleteTimerTask(string name) {
    auto timerTask = timerTasks.find(name);
    if (timerTask != timerTasks.end()) {
        delete timerTask->second;
        timerTasks.erase(timerTask++);
    }
}

void FusionServer::deleteTimerTaskAll() {
    auto iter = timerTasks.begin();
    while (iter != timerTasks.end()) {
        delete iter->second;
        iter = timerTasks.erase(iter);
    }
}

void FusionServer::TaskFindOneFrame_FusionData(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitFusionData;
    if (server->isRun.load()) {
        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        DataUnitFusionData::MergeType mergeType;
        if (server->isMerge) {
            mergeType = DataUnitFusionData::Merge;
        } else {
            mergeType = DataUnitFusionData::NotMerge;
        }

        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitFusionData::TaskProcessOneFrame, mergeType);
    }
}

void FusionServer::TaskFindOneFrame_TrafficFlow(void *pServer, unsigned int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitTrafficFlows;
    if (server->isRun.load()) {
        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitTrafficFlows::TaskProcessOneFrame);
    }
}

//寻找头部
void FusionServer::TaskFindOneFrame_LineupInfoGather(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitLineupInfoGather;
    if (server->isRun.load()) {
        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitLineupInfoGather::TaskProcessOneFrame);
    }
}

//寻找尾部
void FusionServer::TaskFindOneFrame_CrossTrafficJamAlarm(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitCrossTrafficJamAlarm;
    if (server->isRun.load()) {
        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame, false);
    }
}

void FusionServer::TaskFindOneFrame_MultiViewCarTracks(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitMultiViewCarTracks;
    if (server->isRun.load()) {
        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitMultiViewCarTracks::TaskProcessOneFrame);
    }
}


