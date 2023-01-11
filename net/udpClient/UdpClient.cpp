//
// Created by lining on 9/30/22.
//

#include <unistd.h>
#include <thread>
#include <arpa/inet.h>
#include <cstring>
#include "UdpClient.h"

UdpClient::UdpClient(string ip, int port, string name) :
        serverIp(ip), serverPort(port), name(name) {

}

UdpClient::~UdpClient() {
}

int UdpClient::Open() {
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

int UdpClient::Close() {
    isRun = false;
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (isLocalThreadRun) {
        try {
            futureGetRecv.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    isLocalThreadRun = false;
    return 0;
}

void UdpClient::Run() {
    isLocalThreadRun = true;
    futureGetRecv = std::async(std::launch::async, ThreadGetRecv, this);
}

int UdpClient::Send(uint8_t *data, uint32_t len) {
    socklen_t addrLen = sizeof(struct sockaddr_in);

    int nleft = 0;
    int nsend = 0;
    if (sock) {
        pthread_mutex_lock(&lockSend);
        uint8_t *ptr = data;
        nleft = len;
        while (nleft > 0) {
            if ((nsend = sendto(sock, ptr, nleft, 0, (struct sockaddr *) &server_addr, addrLen)) < 0) {
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

int UdpClient::createSock() {
    int socketFd;
    // Create UDP socket:
    socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketFd < 0) {
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successful\n");

    // Set port and IP:
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(serverPort);
    server_addr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    return socketFd;
}

int UdpClient::ThreadGetRecv(void *p) {
    if (p == nullptr) {
        return -1;
    }

    UdpClient *client = (UdpClient *) p;
    socklen_t sockaddrLen = sizeof(struct sockaddr_in);
    printf("udp client run\n");

    uint8_t *msg = new uint8_t[1024 * 1024];
    while (client->isRun) {
        int len = recvfrom(client->sock, msg, (1024 * 1024), 0,
                           (struct sockaddr *) &client->server_addr, &sockaddrLen);
        if (len > 0) {
            client->OnRecv(msg, len);
        }
    }
    delete[] msg;
    printf("udp client exit\n");
    return 0;
}
