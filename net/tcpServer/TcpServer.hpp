//
// Created by lining on 1/11/23.
//

#ifndef _TCPSERVER_HPP
#define _TCPSERVER_HPP

#include <string>
#include <vector>
#include <future>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "TcpServerClient.hpp"

using namespace std;

template<class TcpServerClient>
class TcpServer {

public:
    int sock = 0;
    int port;
    string name;
    int maxListen = 10;

    bool isRun = false;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureRun;

    vector<TcpServerClient *> clients;
    pthread_mutex_t lockClients = PTHREAD_MUTEX_INITIALIZER;

protected:
    //epoll
    int epoll_fd = 0;
    struct epoll_event ev;
#define MAX_EVENTS 1024
    struct epoll_event wait_events[MAX_EVENTS];

public:
    TcpServer(int port, string name) : port(port), name(name) {

    }

    ~TcpServer() {

    }

private:

    int createSock() {
        //1.申请sock
        int sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockFd == -1) {
            printf("Server:%d sock fail:%s\n", this->port, strerror(errno));
            return -1;
        }
        //2.设置地址、端口
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(this->port);
        //将套接字绑定到服务器的网络地址上
        if (bind(sockFd, (struct sockaddr *) &server_addr, (socklen_t) sizeof(server_addr)) < 0) {
            printf("Server:%d sock bind error:%s\n", this->port, strerror(errno));
            close(sockFd);
            return -1;
        }

        //3.设置属性
        int opt = 1;
        setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(sockFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));


//    struct timeval timeout;
//    timeout.tv_sec = 3;
//    timeout.tv_usec = 0;
//    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
//    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

        //4.监听
        if (listen(sockFd, maxListen) != 0) {
            printf("Server:%d sock listen fail,error:%s\n", this->port, strerror(errno));
            close(sockFd);
            return -1;
        }
        return sockFd;
    }

public:
    int EpollInit() {
        //5.创建一个epoll句柄
        epoll_fd = epoll_create(MAX_EVENTS);
        if (epoll_fd == -1) {
            printf("epoll create fail\n");
            return -1;
        }
        ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
        ev.data.fd = sock;
        // 向epoll注册server_sockfd监听事件
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) == -1) {
            printf("epoll_ctl Server fd failed:%s\n", strerror(errno));
            return -1;
        }

        return 0;
    }


    int EpollAdd(int client_fd) {
        this->ev.events = EPOLLHUP | EPOLLET | EPOLLERR | EPOLLRDHUP;
        this->ev.data.fd = client_fd;
        if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, client_fd, &this->ev) == -1) {
            printf("epoll add:%d fail,%s\n", client_fd, strerror(errno));
            return -1;
        }
        return 0;
    }

    int EpollDel(int client_fd) {
        if (epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, client_fd, &this->ev) == -1) {
            printf("epoll del %d fail,%s\n", client_fd, strerror(errno));
            return -1;
        }
        return 0;
    }

private:
    /**
    * 有客户端连入回调
    * @param clientSock
    * @param clientSockAddr
    */
    void cbConnect(int clientSock, struct sockaddr_in clientSockAddr) {
        string name = "serverClient " + to_string(clientSock);
        //将客户端设置为nonblock
        fcntl(clientSock, F_SETFL, O_NONBLOCK);
        TcpServerClient *client = new TcpServerClient(clientSock, clientSockAddr, name, this);
        printf("client %s connect", inet_ntoa(client->addr.sin_addr));
        pthread_mutex_lock(&lockClients);
        clients.push_back(client);
        pthread_mutex_unlock(&lockClients);
        client->Run();
    }

    /**
     * 有客户端断开回调
     * @param clientSock
     * @param clientSockAddr
     */
    void cbDisconnect(int clientSock) {
        pthread_mutex_lock(&lockClients);
        for (int i = 0; i < clients.size(); i++) {
            auto iter = clients.at(i);
            if (iter->sock == clientSock) {
                delete iter;
                clients.erase(clients.begin() + i);
            }
        }
        clients.shrink_to_fit();
        pthread_mutex_unlock(&lockClients);
    }


    static int ThreadRun(void *local) {
        if (local == nullptr) {
            return -1;
        }
        TcpServer *server = (TcpServer *) local;

        int nfds = 0;// epoll监听事件发生的个数
        struct sockaddr_in remote_addr; // 客户端网络地址结构体
        socklen_t sin_size = sizeof(struct sockaddr_in);
        int client_fd;

        string name = server->name;
        cout << name << " " << __FUNCTION__ << " run" << endl;
        while (server->isRun) {
            // 等待事件发生
            nfds = epoll_wait(server->epoll_fd, server->wait_events, MAX_EVENTS, -1);

            if (nfds == -1) {
                continue;
            }
            for (int i = 0; i < nfds; i++) {
                //外部只处理连接和断开状态
                if (server->wait_events[i].data.fd == server->sock) {
                    // 客户端有新的连接请求
                    // 等待客户端连接请求到达
                    client_fd = accept(server->sock, (struct sockaddr *) &remote_addr, &sin_size);
                    if (client_fd < 0) {
                        printf("Server accept fail:%s\n", strerror(errno));
                        continue;
                    }
                    printf("accept client:%d,ip:%s\n", client_fd, inet_ntoa(remote_addr.sin_addr));
                    server->EpollAdd(client_fd);
                    server->cbConnect(client_fd, remote_addr);

                } else if ((server->wait_events[i].data.fd > 2) &&
                           ((server->wait_events[i].events & EPOLLERR) ||
                            (server->wait_events[i].events & EPOLLRDHUP) ||
                            (server->wait_events[i].events & EPOLLHUP))) {
                    //有异常发生 close client
                    client_fd = server->wait_events[i].data.fd;
                    //epoll del
                    server->EpollDel(client_fd);
                    server->cbDisconnect(client_fd);
                }
            }
        }
        cout << name << " " << __FUNCTION__ << " exit" << endl;
        return 0;
    }

public:
    int Open() {
        int fd = createSock();
        if (fd > 0) {
            sock = fd;
            EpollInit();
            isRun = true;
        } else {
            isRun = false;
            return -1;
        }
        return 0;
    }

    int DeleteClient(int sock) {
        //删除客户端数组
        if (!clients.empty()) {

            pthread_mutex_lock(&lockClients);

            for (auto &iter:clients) {
                delete iter;
            }
            clients.clear();

            auto iter = clients.begin();
            do {
                if (static_cast<TcpServerClient *>(*iter)->sock == sock) {
                    delete *iter;
                    iter = clients.erase(iter);
                }
            } while (iter != clients.end());
            clients.shrink_to_fit();

            pthread_mutex_unlock(&lockClients);
        }
        return 0;
    }


    void DeleteAllClient() {
        //删除客户端数组
        if (!clients.empty()) {

            pthread_mutex_lock(&lockClients);

            for (auto &iter:clients) {
                delete iter;
            }
            clients.clear();
            pthread_mutex_unlock(&lockClients);
        }

    }

    int Close() {
        isRun = false;
        if (sock > 0) {
            shutdown(sock, SHUT_RDWR);
            close(sock);
            sock = 0;
        }
        if (isLocalThreadRun) {
            try {
                futureRun.wait();
            } catch (std::future_error e) {
                printf("%s %s\n", __FUNCTION__, e.what());
            }
        }
        isLocalThreadRun = false;
        DeleteAllClient();
        if (epoll_fd > 0) {
            close(epoll_fd);
            epoll_fd = 0;
        }
        return 0;
    }

    int Run() {
        if (!isRun) {
            return -1;
        }
        isLocalThreadRun = true;
        futureRun = std::async(std::launch::async, ThreadRun, this);
        return 0;
    }
};


#endif //_TCPSERVER_HPP
