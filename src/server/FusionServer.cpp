//
// Created by lining on 2/15/22.
//

#include <fcntl.h>
#include <iostream>
#include <log/Log.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "server/FusionServer.h"
#include "merge/merge.h"
#include "simpleini/SimpleIni.h"

using namespace log;

FusionServer::FusionServer() {
    this->isRun.store(false);
}

FusionServer::FusionServer(uint16_t port, string config, int maxListen) {
    this->port = port;
    this->config = config;
    this->maxListen = maxListen;
    this->isRun.store(false);
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
        const char *S_widthX = ini.GetValue("server", "widthX", "");
        widthX = atof(S_widthX);
        const char *S_widthY = ini.GetValue("server", "widthY", "");
        widthY = atof(S_widthY);
        const char *S_Xmax = ini.GetValue("server", "Xmax", "");
        Xmax = atof(S_Xmax);
        const char *S_Ymax = ini.GetValue("server", "Ymax", "");
        Ymax = atof(S_Ymax);
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

int FusionServer::Open() {
    //1.申请sock
    sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
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

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    int recvBufSize = 128 * 1024;
    int sendBufSize = 128 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));

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

    return 0;
}

int FusionServer::Run() {

    if (sock <= 0) {
        Error("server sock:%d", sock);
        return -1;
    }
    isRun.store(true);

    //开启服务器监视客户端线程
    threadMonitor = thread(ThreadMonitor, this);
    pthread_setname_np(threadMonitor.native_handle(), "FusionServer monitor");
    threadMonitor.detach();

    //开启服务器检查客户端线程
    threadCheck = thread(ThreadCheck, this);
    pthread_setname_np(threadCheck.native_handle(), "FusionServer check");
    threadCheck.detach();

    //开启服务器获取一帧大数据线程
    threadFindOneFrame = thread(ThreadFindOneFrame, this);
    pthread_setname_np(threadFindOneFrame.native_handle(), "FusionServer findOneFrame");
    threadFindOneFrame.detach();

    //开启服务器多路数据融合线程
    threadMerge = thread(ThreadMerge, this);
    pthread_setname_np(threadMerge.native_handle(), "FusionServer merge");
    threadMerge.detach();

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

    //clientInfo
    ClientInfo *clientInfo = new ClientInfo(remote_addr, client_sock);

    //add vector
    pthread_mutex_lock(&this->lock_vector_client);
    this->vector_client.push_back(clientInfo);
    pthread_cond_broadcast(&this->cond_vector_client);
    pthread_mutex_unlock(&this->lock_vector_client);

    Info("add client %d success", client_sock);

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
    pthread_mutex_lock(&this->lock_vector_client);
    //vector erase
    for (int i = 0; i < vector_client.size(); i++) {
        auto iter = vector_client.at(i);
        if (iter->sock == client_sock) {

            //delete client class  close sock release clientInfo->rb
            delete iter;

            //erase vector
            vector_client.erase(vector_client.begin() + i);
            vector_client.shrink_to_fit();

            Info("remove client:%d", client_sock);
            break;
        }
    }
    pthread_cond_broadcast(&this->cond_vector_client);
    pthread_mutex_unlock(&this->lock_vector_client);

    return 0;
}

int FusionServer::DeleteAllClient() {

    //删除客户端数组
    if (!vector_client.empty()) {

        pthread_mutex_lock(&lock_vector_client);

        if (vector_client.empty()) {
            pthread_cond_wait(&cond_vector_client, &lock_vector_client);
        }

        for (auto iter:vector_client) {
            if (iter->sock > 0) {
                close(iter->sock);
                iter->sock = 0;
            }
            delete iter;
        }
        vector_client.clear();
        pthread_cond_broadcast(&cond_vector_client);
        pthread_mutex_unlock(&lock_vector_client);
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

                for (int j = 0; j < server->vector_client.size(); j++) {
                    auto iter = server->vector_client.at(j);
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

    Info("%s exit", __FUNCTION__);
}

void FusionServer::ThreadCheck(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {
        usleep(10);
        //检查客户端数组状态，
        // 主要是接收时间receive_time receive_time跟现在大于150秒则断开链接
        // needRelease上抛的释放标志，设置为真后，等待队列全部为空后进行释放
        if (server->vector_client.empty()) {
            continue;
        }
//        cout << "当前客户端数量：" << to_string(server->vector_client.size()) << endl;

        //切记，这里删除客户端的时候有加锁解锁操作，所以最外围不要加锁！！！
//        pthread_mutex_lock(&server->lock_vector_client);
//        if (server->vector_client.empty()) {
//            pthread_cond_wait(&server->cond_vector_client, &server->lock_vector_client);
//        }

        for (auto iter: server->vector_client) {

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
                    bool rbEmpty = false;//字节缓冲区为空
                    //如果队列全部为空，从数组中删除
                    if (iter->rb != nullptr) {
                        if (iter->rb->available_for_read == 0) {
                            rbEmpty = true;
                        }
                    }
                    bool pkgEmpty = false;//分包缓冲区为空
                    if (iter->queuePkg.Size() == 0) {
                        pkgEmpty = true;
                    }
                    bool watchDataEmpty = false;//WatchData队列为空
                    if (iter->queueWatchData.size() == 0) {
                        watchDataEmpty = true;
                    }

                    //都为空则从数组删除
                    if (rbEmpty && pkgEmpty && watchDataEmpty) {
                        server->RemoveClient(iter->sock);
                    }
                }
            }
        }

//        pthread_cond_broadcast(&server->cond_vector_client);
//        pthread_mutex_unlock(&server->lock_vector_client);

    }

    Info("%s exit", __FUNCTION__);

}

void FusionServer::ThreadFindOneFrame(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    struct timespec begin;
    struct timespec end;
    uint64_t timeUsConsume = 0;

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {
        Info("耗时:%llu", timeUsConsume);

        if (timeUsConsume > (100 * 1000)) {
            timeUsConsume = timeUsConsume % (100 * 1000);
        }
        usleep(server->thresholdFrame * 1000 - timeUsConsume);

        clock_gettime(CLOCK_REALTIME, &begin);
        //如果连接上的客户端数量为0
        if (server->vector_client.size() == 0) {
            Info("未连入客户端");
            clock_gettime(CLOCK_REALTIME, &end);
            timeUsConsume = ((end.tv_sec * 1000 * 1000 + end.tv_nsec / 1000) -
                             (begin.tv_sec * 1000 * 1000 + begin.tv_nsec / 1000));
            continue;
        }
        //所有的客户端都要缓存3帧数据
        int clientNum = server->vector_client.size();//连入的客户端数量
        int has3PkgCount = 0;//有3帧缓存数据的路数
        bool hasData = false;
        for (int i = 0; i < server->vector_client.size(); i++) {
            auto iter = server->vector_client.at(i);
            if (iter->queueWatchData.size() >= 3) {
                has3PkgCount += 1;
            }
        }

        Info("clientNum %d,has3PkgCount %d", clientNum, has3PkgCount);

        if (has3PkgCount > 0) {
            hasData = true;
        }

        if (!hasData) {
            Info("全部路数未有3帧数据缓存");
            clock_gettime(CLOCK_REALTIME, &end);
            timeUsConsume = ((end.tv_sec * 1000 * 1000 + end.tv_nsec / 1000) -
                             (begin.tv_sec * 1000 * 1000 + begin.tv_nsec / 1000));
            continue;
        }

        OBJS objs;//容量为4,依次是南、西、北、东
        uint64_t timestamp = 0;//时间戳，选取时间最近的方向

        //轮询各个连入的客户端，按指定的路口ip来存入数组
        pthread_mutex_lock(&server->lock_vector_client);
        if (server->vector_client.empty()) {
            pthread_cond_wait(&server->cond_vector_client, &server->lock_vector_client);
        }

        int id[4] = {-1, -1, -1, -1};//以方向为目的存入的路口方向对应的客户端数组的索引
        //1.寻找各个路口的客户端索引，未找到就是默认值-1
        for (int i = 0; i < server->vector_client.size(); i++) {
            auto iter = server->vector_client.at(i);
            for (int j = 0; j < server->roadDirection.size(); j++) {
                //ip 相同
                if (iter->direction == server->roadDirection.at(j)) {
                    id[j] = i;
                }
            }
        }

        //调试输出，查看现在有几路连入
        bool isConnect = false;
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] != -1) {
                Info("第%d路 client数组索引%d sock：%d", i + 1, id[i], server->vector_client.at(id[i])->sock);
                isConnect = true;
            }
        }
        if (!isConnect) {
            clock_gettime(CLOCK_REALTIME, &end);
            timeUsConsume = ((end.tv_sec * 1000 * 1000 + end.tv_nsec / 1000) -
                             (begin.tv_sec * 1000 * 1000 + begin.tv_nsec / 1000));
            continue;
        }


        //2.确定时间戳
        if (server->curTimestamp == 0) {
            //第一次取先遍历的帧时间
            for (int i = 0; i < ARRAY_SIZE(id); i++) {
                if (id[i] == -1) {
                    continue;
                }
                if (!server->vector_client.at(id[i])->queueWatchData.empty()) {
                    //这里取各个客户端第一帧的最大值
                    if (server->curTimestamp < server->vector_client.at(id[i])->queueWatchData.front().timstamp) {
                        //如果还没有赋值则赋值
                        server->curTimestamp = server->vector_client.at(id[i])->queueWatchData.front().timstamp;
                    }
                }
            }
        } else {
            //从第二次开始，暂时定是上一帧的时间加上预设的时间间隔
            server->curTimestamp += server->thresholdFrame;
        }

        //2.根据时间戳和门限来赋值
        vector<OBJECT_INFO_T> obj[4];
        for (int i = 0; i < ARRAY_SIZE(obj); i++) {
            obj[i].clear();
        }
        //清理第N路的取帧时间戳记录
        for (int i = 0; i < ARRAY_SIZE(server->xRoadTimestamp); i++) {
            server->xRoadTimestamp[i] = 0;
        }

        //取数据
        for (int i = 0; i < ARRAY_SIZE(id); i++) {
            if (id[i] == -1) {
                //客户端未连入，不填充数据
            } else {
                //客户端连入，填充数据
                //watchData数据
                if (server->vector_client.at(id[i])->queueWatchData.empty()) {
                    //客户端未有数据传入，不填充数据
                    Info("客户端%d未有数据传入，不填充数据", server->vector_client.at(id[i])->sock);
                } else {
                    //找到门限内的数据,1.只有一组数据，则就是它了，如果多组则选取门限内的
                    bool isFind = false;
                    do {
                        if (server->vector_client.at(id[i])->queueWatchData.size() == 1) {
                            auto iter = server->vector_client.at(id[i])->queueWatchData.front();
                            //按路记录时间戳
                            server->xRoadTimestamp[i] = iter.timstamp;
                            Info("第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                            //TODO 赋值路口编号
                            if (server->crossID.empty()) {
                                server->crossID = iter.matrixNo;
                            }
                            for (auto item: iter.lstObjTarget) {
                                OBJECT_INFO_T objectInfoT;
                                //转换
                                ObjTarget2OBJECT_INFO_T(item, objectInfoT);
                                obj[i].push_back(objectInfoT);
                            }
                            //出队列
                            server->vector_client.at(id[i])->queueWatchData.pop();
                            isFind = true;
                        } else {
                            auto iter = server->vector_client.at(id[i])->queueWatchData.front();
                            if (iter.timstamp > server->curTimestamp) {
                                //按路记录时间戳
                                server->xRoadTimestamp[i] = iter.timstamp;
                                Info("第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                //TODO 赋值路口编号
                                if (server->crossID.empty()) {
                                    server->crossID = iter.matrixNo;
                                }
                                //时间戳大于当前时间戳，直接取
                                for (auto item: iter.lstObjTarget) {
                                    OBJECT_INFO_T objectInfoT;
                                    //转换
                                    ObjTarget2OBJECT_INFO_T(item, objectInfoT);
                                    obj[i].push_back(objectInfoT);
                                }
                                //出队列
                                server->vector_client.at(id[i])->queueWatchData.pop();
                                isFind = true;
                            } else if (iter.timstamp >= (server->curTimestamp - server->thresholdFrame)) {
                                //按路记录时间戳
                                server->xRoadTimestamp[i] = iter.timstamp;
                                Info("第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                                //TODO 赋值路口编号
                                if (server->crossID.empty()) {
                                    server->crossID = iter.matrixNo;
                                }
                                //时间戳在门限范围内,赋值
                                for (auto item: iter.lstObjTarget) {
                                    OBJECT_INFO_T objectInfoT;
                                    //转换
                                    ObjTarget2OBJECT_INFO_T(item, objectInfoT);
                                    obj[i].push_back(objectInfoT);
                                }
                                //出队列
                                server->vector_client.at(id[i])->queueWatchData.pop();
                                isFind = true;
                            } else {
                                //小于当前时间戳或者不在门限内，出队列抛弃
                                Info("第%d路时间戳不符合要求，舍弃%lu", i + 1,
                                     server->vector_client.at(id[i])->queueWatchData.front().timstamp);
                                //出队列
                                server->vector_client.at(id[i])->queueWatchData.pop();
                            }
                        }
                    } while (!isFind);
                }
            }
        }
        //3.存入待融合队列，即queueObjs
        for (int i = 0; i < obj[0].size(); i++) {
            objs.one.push_back(obj[0].at(i));
        }
        for (int i = 0; i < obj[1].size(); i++) {
            objs.two.push_back(obj[1].at(i));
        }
        for (int i = 0; i < obj[2].size(); i++) {
            objs.three.push_back(obj[2].at(i));
        }
        for (int i = 0; i < obj[3].size(); i++) {
            objs.four.push_back(obj[3].at(i));
        }


        //计算所有能取到帧的路的时间戳的平均值，赋值给当前时间戳(这里想法是做一个时间戳的加权，动态适应)
        uint64_t sum = 0;
        int num = 0;
        for (int i = 0; i < ARRAY_SIZE(server->xRoadTimestamp); i++) {
            if (server->xRoadTimestamp[i] != 0) {
                sum += server->xRoadTimestamp[i];
                num += 1;
            }
        }
        if (num != 0) {
            Info("sum:%lu,num:%d\n", sum, num);
            server->curTimestamp = (sum / num);
            Info("计算的加权平均数(剔除未赋值的):%lu", server->curTimestamp);
        }

        objs.timestamp = server->curTimestamp;


        if (objs.one.empty() && objs.two.empty() && objs.three.empty() && objs.four.empty()) {
            Info("同一帧数据全部为空");
        } else {
            if (server->queueObjs.Push(objs) != 0) {
                Error("队列已满，未存入数据 timestamp:%lu", server->curTimestamp);
            } else {
                Info("待融合数据存入:选取的时间戳:%lu", server->curTimestamp);
                for (int i = 0; i < ARRAY_SIZE(server->xRoadTimestamp); i++) {
                    if (server->xRoadTimestamp[i] != 0) {
                        Info("第%d路取的时间戳是%lu", i + 1, server->xRoadTimestamp[i]);
                    }
                }
            }
        }

        pthread_cond_broadcast(&server->cond_vector_client);
        pthread_mutex_unlock(&server->lock_vector_client);

        clock_gettime(CLOCK_REALTIME, &end);
        timeUsConsume = ((end.tv_sec * 1000 * 1000 + end.tv_nsec / 1000) -
                         (begin.tv_sec * 1000 * 1000 + begin.tv_nsec / 1000));

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
        usleep(10);

        if (server->queueObjs.Size() == 0) {
            continue;
        }

        //开始融合数据
        OBJS objs = server->queueObjs.PopFront();

        vector<OBJECT_INFO_NEW> vectorOBJNEW;
        OBJECT_INFO_NEW dataOut[1000];
        memset(dataOut, 0, ARRAY_SIZE(dataOut) * sizeof(OBJECT_INFO_NEW));

        switch (server->frame) {
            case 1: {
                //第一帧
                server->l1_obj.clear();
                server->l2_obj.clear();

                int num = merge_total(server->repateX, server->widthX, server->widthY, server->Xmax, server->Ymax,
                                      server->gatetx, server->gatety, server->gatex, server->gatey, true,
                                      objs.one.data(), objs.one.size(), objs.two.data(), objs.two.size(),
                                      objs.three.data(), objs.three.size(), objs.four.data(), objs.four.size(),
                                      server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                      server->l2_obj.size(), dataOut, server->angle_value);
                //传递历史数据
                server->l2_obj.assign(server->l1_obj.begin(), server->l1_obj.end());

                for (int i = 0; i < num; i++) {
                    server->l1_obj.push_back(dataOut[i]);
                }
                server->frame++;

                for (int i = 0; i < num; i++) {
                    vectorOBJNEW.push_back(dataOut[i]);
                }
                MergeData mergeData;
                mergeData.timestamp = objs.timestamp;
                mergeData.obj.clear();
                mergeData.obj.assign(vectorOBJNEW.begin(), vectorOBJNEW.end());

                if (server->queueMergeData.Push(mergeData) != 0) {
                    Error("队列已满，抛弃融合数据 ");
                } else {
                    Info("存入融合数据，size:%d", mergeData.obj.size());
                }
            }
                break;
            default: {
                int num = merge_total(server->repateX, server->widthX, server->widthY, server->Xmax, server->Ymax,
                                      server->gatetx, server->gatety, server->gatex, server->gatey, true,
                                      objs.one.data(), objs.one.size(), objs.two.data(), objs.two.size(),
                                      objs.three.data(), objs.three.size(), objs.four.data(), objs.four.size(),
                                      server->l1_obj.data(), server->l1_obj.size(), server->l2_obj.data(),
                                      server->l2_obj.size(), dataOut, server->angle_value);
                //传递历史数据
                server->l2_obj.assign(server->l1_obj.begin(), server->l1_obj.end());
                for (int i = 0; i < num; i++) {
                    server->l1_obj.push_back(dataOut[i]);
                }

                server->frame++;

                if (server->frame > 2) {
                    server->frame = 2;
                }

                for (int i = 0; i < num; i++) {
                    vectorOBJNEW.push_back(dataOut[i]);
                }
                MergeData mergeData;
                mergeData.timestamp = objs.timestamp;
                mergeData.obj.clear();
                mergeData.obj.assign(vectorOBJNEW.begin(), vectorOBJNEW.end());

                if (server->queueMergeData.Push(mergeData) != 0) {
                    Error("队列已满，抛弃融合数据 ");
                } else {
                    Info("存入融合数据，size:%d", mergeData.obj.size());
                }
            }
                break;
        }
    }
    Info("%s exit", __FUNCTION__);
}
