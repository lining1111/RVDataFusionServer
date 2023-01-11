//
// Created by lining on 9/30/22.
//

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/time.h>
#include "TcpClient.h"

using namespace std;

bool TcpClient::GetConnectState() {
    return isConnect;
}

TcpClient::TcpClient(string ip, int port, string name, timeval *readTimeout) :
        serverIp(ip), serverPort(port), name(name) {
    if (readTimeout != nullptr) {
        isSetRecvTimeout = true;
        readTimeout->tv_sec = readTimeout->tv_sec;
        readTimeout->tv_usec = readTimeout->tv_usec;
    }
}

TcpClient::~TcpClient() {

}

int TcpClient::Open() {
    int fd = createSock();
    printf("fd:%d\n", fd);
    if (fd > 0) {
        printf("client open success\n");
        sock = fd;
        if (isSetRecvTimeout) {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &recvTimeout, sizeof(struct timeval));
        }

        isConnect = true;
        rb = new RingBuffer(RecvSize);
    } else {
        sock = fd;
        isConnect = false;
        return -1;
    }

    return 0;
}

int TcpClient::Close() {
    isConnect = false;
    if (sock > 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (isLocalThreadRun) {
        try {
            futureDump.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    isLocalThreadRun = false;
    delete rb;
    return 0;
}

int TcpClient::Run() {
    isLocalThreadRun = true;
    futureDump = std::async(std::launch::async, ThreadDump, this);
    return 0;
}

int TcpClient::createSock() {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        cout << "create socket failed" << endl;
        return -1;
    } else {
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }

    struct sockaddr_in server_addr;
    int ret;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(this->serverIp.c_str());
    server_addr.sin_port = htons(this->serverPort);

    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    timeval tv_now;
    gettimeofday(&tv_now, nullptr);

    if (ret == -1) {
        cout << "connect Server:" << this->serverIp << " port: " << to_string(this->serverPort) << " failed"
             << ",at " << ctime((time_t *) &tv_now.tv_sec) << endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int TcpClient::ThreadDump(void *local) {
    if (local == nullptr) {
        return -1;
    }
    auto client = (TcpClient *) local;
    fd_set rfds;
    string name = client->name;
    cout << name << " " << __FUNCTION__ << " run" << endl;

    char *buf = new char[1024 * 1024];
    while (client->isConnect) {
        usleep(10);
        if (client->isAsynReceive) {
            int recvLen = (client->rb->GetWriteLen() < (1024 * 1024)) ? client->rb->GetWriteLen() : (1024 * 1024);
            int len = recv(client->sock, buf, recvLen, 0);
            if (len < 0) {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }
                cout << "recv info error,errno:" << to_string(errno) << endl;
                client->isConnect = false;
            } else if (len > 0) {
                gettimeofday(&client->receive_time, nullptr);
                //把接受数据放到rb
                client->rb->Write(buf,len);
            }
        }
    }
    delete[] buf;
    cout << name << " " << __FUNCTION__ << " exit" << endl;
    return 0;
}

int TcpClient::Send(uint8_t *data, uint32_t len) {
    int nleft = 0;
    int nsend = 0;
    if (sock) {
        pthread_mutex_lock(&lockSend);
        uint8_t *ptr = data;
        nleft = len;
        while (nleft > 0) {
            if ((nsend = send(sock, ptr, nleft, 0)) < 0) {
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
