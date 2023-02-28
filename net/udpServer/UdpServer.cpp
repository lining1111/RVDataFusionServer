//
// Created by lining on 9/30/22.
//

#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "UdpServer.h"

UdpServer::UdpServer(int port, string name) : port(port), name(name) {

}

UdpServer::~UdpServer() {

}

int UdpServer::Open() {
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

int UdpServer::Close() {
    isRun = false;
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (isLocalThreadRun) {
        try {
            futureGetRecv.wait();
        } catch (std::future_error e) {
            printf("%s %s", __FUNCTION__, e.what());
        }
    }
    isLocalThreadRun = false;
    return 0;
}

void UdpServer::Run() {
    isLocalThreadRun = true;
    futureGetRecv = std::async(std::launch::async, ThreadGetRecv, this);
}

int UdpServer::createSock() {
    int socketFd;
    // Create UDP socket:
    socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketFd < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");

    // Set port and IP:
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind to the set port and IP:
    if (bind(socketFd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");

    return socketFd;
}

int UdpServer::ThreadGetRecv(void *p) {
    if (p== nullptr){
        return -1;
    }
    UdpServer *server = (UdpServer *) p;
    socklen_t sockaddrLen = sizeof(struct sockaddr_in);
    uint8_t *msg = new uint8_t[1024 * 1024];
    while (server->isRun) {
        int len = recvfrom(server->sock, msg, (1024 * 1024), 0,
                           (struct sockaddr *) &server->client_addr, &sockaddrLen);
        if (len > 0) {
            server->OnRecv(msg, len);
        }
    }
    delete[] msg;
    return 0;
}

int UdpServer::Send(uint8_t *data, uint32_t len, struct sockaddr_in addr) {
    socklen_t addrLen = sizeof(struct sockaddr_in);

    int nleft = 0;
    int nsend = 0;
    if (sock) {
        pthread_mutex_lock(&lockSend);
        uint8_t *ptr = data;
        nleft = len;
        while (nleft > 0) {
            if ((nsend = sendto(sock, ptr, nleft, 0, (struct sockaddr *) &addr, addrLen)) < 0) {
                if (errno == EINTR) {
                    nsend = 0;          /* and call send() again */
                } else {
                    printf("消息data='%s' nsend=%d, 错误代码是%d\n", data, nsend, errno);
                    pthread_mutex_unlock(&lockSend);
                    return (-1);
                }
            } else if (nsend == 0) {
                break;                  /* EOF */
            }
            nleft -= nsend;
            ptr += nsend;
        }
        pthread_mutex_unlock(&lockSend);
    }

    return nsend;
}
