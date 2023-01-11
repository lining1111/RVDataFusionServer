//
// Created by lining on 10/1/22.
//

#include <unistd.h>
#include <thread>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include "UdpSvrBroadcast.h"

UdpSvrBroadcast::UdpSvrBroadcast(int port, string name, string broadcastContent, int interval) : port(port), name(name),
                                                                                                 content(broadcastContent),
                                                                                                 interval(interval) {

}

UdpSvrBroadcast::~UdpSvrBroadcast() {

}

int UdpSvrBroadcast::Open() {
    int sockFd = createSock();
    if (sockFd > 0) {
        sock = sockFd;
        isRun = true;
    } else {
        isRun = false;
        return -1;
    }
    return 0;
}

int UdpSvrBroadcast::Close() {
    isRun = false;
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (isLocalThreadRun) {
        try {
            futureBroadcast.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    isLocalThreadRun = false;
    return 0;
}

void UdpSvrBroadcast::Run() {
    isLocalThreadRun = true;
    futureBroadcast = std::async(std::launch::async, ThreadBroadcast, this);
}

int UdpSvrBroadcast::createSock() {
    int socketFd;
    // Create UDP socket:
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFd < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
//    server_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    //设置套接字类型
    int opt = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, (char *) &opt, sizeof(opt)) == -1) {
        printf("setsockopt fail,err:%s\n", strerror(errno));
        return -1;
    }

    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

    // Bind to the set port and IP:
    if (bind(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");

    return socketFd;

}

int UdpSvrBroadcast::ThreadBroadcast(void *p) {
    UdpSvrBroadcast *server = (UdpSvrBroadcast *) p;
    socklen_t sockaddrLen = sizeof(struct sockaddr_in);
    while (server->isRun) {
        int ret = sendto(server->sock, server->content.data(), server->content.length(), 0,
                         (sockaddr *) &server->server_addr, sockaddrLen);//向广播地址发布消息
        if (ret < 0) {
            printf("send error...%s\n", strerror(errno));
        } else {
            printf("ok\n");
        }
        sleep(server->interval);
    }
}
