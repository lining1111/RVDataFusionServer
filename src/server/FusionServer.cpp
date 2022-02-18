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

using namespace log;

FusionServer::FusionServer() {
    this->isRun.store(false);
}

FusionServer::FusionServer(uint16_t port, int maxListen) {
    this->port = port;
    this->maxListen = maxListen;
    this->isRun.store(false);
}

FusionServer::~FusionServer() {
    Close();
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

    //开启服务器箭头客户端线程
    threadMonitor = thread(ThreadMonitor, this);
    pthread_setname_np(threadMonitor.native_handle(), "FusionServer monitor");
    threadMonitor.detach();

    //开启服务器箭头客户端线程
    threadCheck = thread(ThreadCheck, this);
    pthread_setname_np(threadCheck.native_handle(), "FusionServer check");
    threadCheck.detach();

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

    Info("accept client:%d,ip:%s", client_sock, inet_ntoa(remote_addr.sin_addr));

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
                //加入客户端数组，开启客户端处理线程
                server->AddClient(client_fd, remote_addr);

            } else if ((server->wait_events[i].data.fd > 2) &&
                       ((server->wait_events[i].events & EPOLLERR) ||
                        (server->wait_events[i].events & EPOLLRDHUP) ||
                        (server->wait_events[i].events & EPOLLOUT))) {
                //有异常发生 close client
                for (int i = 0; i < server->vector_client.size(); i++) {
                    auto iter = server->vector_client.at(i);
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
                    if (iter->queueWatchData.Size() == 0) {
                        watchDataEmpty = true;
                    }

                    //都为空则从数组删除
                    if (rbEmpty && pkgEmpty && watchDataEmpty) {
                        server->RemoveClient(iter->sock);
                    }
                }
            }
        }
    }

    Info("%s exit", __FUNCTION__);

}

void FusionServer::ThreadMerge(void *pServer) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;

    //fix
    const int repateX = 10;
    //不同路口标定值不一样
    const int widthX = 21.3;
    const int widthY = 20;
    const int Xmax = 300;
    const int Ymax = 300;
    const int gatetx = 30;
    const int gatety = 30;
    const int gatex = 10;
    const int gatey = 5;

    //获取路口数据 ip排序严格按照 南 西 北 东
    int roadNum = 4;
    string roadIP[4] = {
            "192.168.1.100",
            "192.168.1.101",
            "192.168.1.102",
            "192.168.1.103",
    };

    Info("%s run", __FUNCTION__);
    while (server->isRun.load()) {
        usleep(10);
        if (server->vector_client.size() < roadNum) {
            continue;
        }

        pthread_mutex_lock(&server->lock_vector_client);
        if (server->vector_client.empty()) {
            pthread_cond_wait(&server->cond_vector_client, &server->lock_vector_client);
        }

        //1.判断是否所有的路口线程都已连接上
        int connected = 0;
        for (auto iter:server->vector_client) {
            for (int i = 0; i < roadNum; i++) {
                if (string(inet_ntoa(iter->clientAddr.sin_addr)) == roadIP[i]) {
                    connected++;
                }
            }
        }
        if (connected < roadNum) {
            pthread_cond_broadcast(&server->cond_vector_client);
            pthread_mutex_unlock(&server->lock_vector_client);
            continue;
        }

        //2.判断所有的路口WatchData队列都有数据,至少3帧
        int hasData = 0;
        for (auto iter:server->vector_client) {
            if (iter->queueWatchData.Size() > 3) {
                hasData++;
            }
        }
        if (hasData < roadNum) {
            pthread_cond_broadcast(&server->cond_vector_client);
            pthread_mutex_unlock(&server->lock_vector_client);
            continue;
        }

        int frame = 1;//帧计数
        //按取帧先后排序 l1第1帧取的  l2第2帧取的  l3第3帧取的
        OBJECT_INFO_T l1_obj[4];
        OBJECT_INFO_T l2_obj[4];
        OBJECT_INFO_T l3_obj[4];

        switch (frame) {
            case 1:{
                //第一帧，需要清空所有对象
                bzero(l1_obj, ARRAY_SIZE(l1_obj)* sizeof(OBJECT_INFO_T));
                bzero(l2_obj, ARRAY_SIZE(l2_obj)* sizeof(OBJECT_INFO_T));
                bzero(l3_obj, ARRAY_SIZE(l3_obj)* sizeof(OBJECT_INFO_T));
            }
                break;
            case 2:
                break;
        }


        //3.寻找在一帧内的数据，判定标准为几个路口的帧 内的RecordDateTime是否都在50ms误差内
        //判断步骤为，取出4路的头第一包数据，以时间最新的标准，其他路的如果和这个标准误差在50ms外的，
        //认为是过时帧，丢弃，再在同一路取一帧数据
        for (auto iter: server->vector_client) {

        }

        //4.将同一帧的路口数据放入融合函数，进行融合
        //4.1融合步骤，保留前2帧的融合数据，保留前2帧的航角数据
        //一些固定值
        //5.将融合后的数据存入队列


        releaseLock:
        pthread_cond_broadcast(&server->cond_vector_client);
        pthread_mutex_unlock(&server->lock_vector_client);

    }
    Info("%s exit", __FUNCTION__);
}
