//
// Created by lining on 9/30/22.
//

#ifndef _TCPSERVERCLIENT_HPP
#define _TCPSERVERCLIENT_HPP


#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>
#include "ringbuffer/RingBuffer.h"
#include <future>
#include <csignal>

using namespace std;

class TcpServerClient {
public:
    void *super;
    string name;
    int sock;
    struct sockaddr_in addr;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureDump;
public:
    bool isAsynReceive = true;
    pthread_mutex_t lockSend = PTHREAD_MUTEX_INITIALIZER;
    RingBuffer *rb = nullptr;
    timeval receive_time;

    static int ThreadDump(void *p) {
        if (p == nullptr) {
            return -1;
        }
        auto client = (TcpServerClient *) p;

        string name = client->name;
        cout << name << " " << __FUNCTION__ << " run" << endl;
        char *buf = new char[1024 * 1024];
        while (client->isConnect) {
            usleep(10);
            if (client->isAsynReceive) {
                bzero(buf, 1024 * 1024);
                int recvLen = (client->rb->GetWriteLen() < (1024 * 1024)) ? client->rb->GetWriteLen() : (1024 * 1024);
                int len = recv(client->sock, buf, recvLen, 0);
                if (len < 0) {
//                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
//                    continue;
//                }
//                cout << "recv info error,errno:" << strerror(errno) << endl;
//                client->isConnect = false;
                } else if (len > 0) {
                    gettimeofday(&client->receive_time, nullptr);
                    //把接受数据放到rb_cameraRecv
                    client->rb->Write(buf, len);
                }
            }
        }
        cout << name << " " << __FUNCTION__ << " exit" << endl;
        return 0;
    }

    bool isConnect = false;
    bool isClose = false;
public:
    TcpServerClient(int sock, struct sockaddr_in addr, string name, void *super,
                    int recvMax = (1024 * 1024), timeval *readTimeout = nullptr) :
            sock(sock), addr(addr), name(name), super(super) {
        isConnect = true;
        if (readTimeout != nullptr) {
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &readTimeout, sizeof(struct timeval));
        }

        rb = new RingBuffer(recvMax);
    }


    ~TcpServerClient() {
        Close();
    }

public:
    int Close() {
        if (isClose) {
            return 0;
        }
        isConnect = false;
        if (sock > 0) {
            shutdown(sock, SHUT_RDWR);
            close(sock);
            sock = 0;
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
        isClose = true;
    }

    int Run() {
        isLocalThreadRun = true;
        futureDump = std::async(std::launch::async, ThreadDump, this);

        return 0;
    }

    int Send(uint8_t *data, int len) {
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
                        return -1;
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
};


#endif //_TCPSERVERCLIENT_HPP
