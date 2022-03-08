//
// Created by lining on 3/8/22.
//

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include "client/FusionClient.h"
#include "common/common.h"
#include "common/CRC.h"
#include "log/Log.h"

using namespace common;
using namespace log;

FusionClient::FusionClient(string server_ip, unsigned int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

FusionClient::~FusionClient() {
    Close();
}

int FusionClient::Open() {
    rb = RingBuffer_New(RecvSize);
    if (this->server_ip.empty()) {
        cout << "server ip empty" << endl;
        return -1;
    }

    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        cout << "create socket failed" << endl;
        return -1;
    } else {
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

        int recvBufSize = 32 * 1024;
        int sendBufSize = 32 * 1024;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));
    }

    struct sockaddr_in server_addr;
    int ret = 0;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(this->server_port);

    server_addr.sin_addr.s_addr = inet_addr(this->server_ip.c_str());
    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    timeval tv_now;
    gettimeofday(&tv_now, nullptr);

    if (ret == -1) {
        cout << "connect server:" << this->server_ip << " port: " << to_string(this->server_port) << " failed"
             << ",at " << ctime((time_t *) &tv_now.tv_sec) << endl;
        close(sockfd);
        return -1;
    }

    cout << "connect server:" << this->server_ip << " port: " << to_string(this->server_port) << " success"
         << ",at " << ctime((time_t *) &tv_now.tv_sec) << endl;

    this->sockfd = sockfd;
    isRun = true;
    return 0;
}

int FusionClient::Run() {
    if (this->isRun == false) {
        cout << "server not connect" << endl;
        return -1;
    }

    //start pthread
    thread_recv = thread(ThreadRecv, this);
    thread_recv.detach();

    thread_processRecv = thread(ThreadProcessRecv, this);
    thread_processRecv.detach();

    thread_processSend = thread(ThreadProcessSend, this);
    thread_processSend.detach();
    //因为上层不会回复，所以不检查状态
//    thread_checkStatus = thread(ThreadCheckStatus, this);
//    thread_checkStatus.detach();

    return 0;
}

int FusionClient::Close() {
    isRun = false;

    if (sockfd > 0) {
        close(sockfd);
    }
    RingBuffer_Delete(rb);
    rb = nullptr;
    return 0;
}

void FusionClient::ThreadRecv(void *p) {
    if (p == nullptr) {
        return;
    }
    auto client = (FusionClient *) p;

    char buf[1024 * 512] = {0};
    int nread = 0;

    cout << "FusionClient " << __FUNCTION__ << " run" << endl;
    while (client->isRun) {
        usleep(10);
        memset(buf, 0, ARRAY_SIZE(buf));
        nread = recv(client->sockfd, buf, sizeof(buf), 0);
        if (nread < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            cout << "recv info error,errno:" << to_string(errno) << endl;
            close(client->sockfd);
            timeval tv_now;
            gettimeofday(&tv_now, nullptr);
            cout << "close sock:" << to_string(client->sockfd) << ",at " << ctime((time_t *) &tv_now.tv_sec) << endl;
            client->isRun = false;
        } else if (nread > 0) {
            cout << "recv info" << endl;
            //update receive_time
            gettimeofday(&client->receive_time, nullptr);
            //把接受数据放到rb_cameraRecv
            RingBuffer_Write(client->rb, buf, nread);
        }
    }
    cout << "FusionClient " << __FUNCTION__ << " exit" << endl;

}

void FusionClient::ThreadProcessRecv(void *p) {
    if (p == nullptr) {
        return;
    }
    auto client = (FusionClient *) p;

    cout << "Client_Interface " << __FUNCTION__ << " run" << endl;
    while (client->isRun) {

        usleep(10);

        if (client->rb == nullptr) {
            //数据缓存区不存在
            continue;
        }

        //分包状态机
        switch (client->status) {
            case Start: {
                //开始,寻找包头的开始标志
                if (client->rb->available_for_read >= MEMBER_SIZE(PkgHead, tag)) {
                    char start = '\0';
                    RingBuffer_Read(client->rb, &start, 1);
                    if (start == '$') {
                        //找到开始标志
                        client->status = GetHeadStart;
                    }
                }
            }
                break;
            case GetHeadStart: {
                //开始寻找头
                if (client->rb->available_for_read >= (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag))) {
                    //获取完整的包头
                    client->pkgHead.tag = '$';
                    RingBuffer_Read(client->rb, &client->pkgHead.version,
                                    (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag)));
                    //buffer申请内存
                    client->pkgBuffer = (uint8_t *) calloc(1, client->pkgHead.len);
                    client->index = 0;
                    memcpy(client->pkgBuffer, &client->pkgHead, sizeof(pkgHead));
                    client->index += sizeof(pkgHead);
                    client->status = GetHead;
                    client->bodyLen = client->pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                }
            }
                break;
            case GetHead: {
                if (client->bodyLen <= 0) {
                    client->bodyLen = client->pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                }
                if (client->rb->available_for_read >= client->bodyLen) {
                    //获取正文
                    RingBuffer_Read(client->rb, client->pkgBuffer + client->index, client->bodyLen);
                    client->index += client->bodyLen;
                    client->status = GetBody;
                }
            }
                break;
            case GetBody: {
                if (client->rb->available_for_read >= sizeof(PkgCRC)) {
                    //获取CRC
                    RingBuffer_Read(client->rb, client->pkgBuffer + client->index, sizeof(PkgCRC));
                    client->index += sizeof(PkgCRC);
                    client->status = GetCRC;
                }
            }
                break;
            case GetCRC: {
                //获取CRC 就获取全部的分包内容，转换为结构体
                Pkg pkg;

                Unpack(client->pkgBuffer, client->pkgHead.len, pkg);

                //判断CRC是否正确
                //打印下buffer
//                PrintHex(client->pkgBuffer, client->pkgHead.len);
                uint16_t crc = Crc16TabCCITT(client->pkgBuffer, client->pkgHead.len - 2);
                if (crc != pkg.crc.data) {//CRC校验失败
                    Error("CRC fail, 计算值:%d,包内值:%d", crc, pkg.crc.data);
                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        free(client->pkgBuffer);
                        client->pkgBuffer = nullptr;//分包缓冲
                    }
                    client->index = 0;//分包缓冲的索引
                    client->status = Start;
                } else {

                    //记录接包时间
                    gettimeofday(&client->receive_time, nullptr);

                    //存入分包队列
                    if (client->queuePkg.Size() >= client->maxQueuePkg) {
                        Info("client:%d 分包队列已满，丢弃此包:%d-%s", client->sockfd, pkg.head.cmd, pkg.body.c_str());
                    } else {
                        client->queuePkg.Push(pkg);
                    }

                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        free(client->pkgBuffer);
                        client->pkgBuffer = nullptr;//分包缓冲
                    }
                    client->index = 0;//分包缓冲的索引
                    client->status = Start;
                }
            }
                break;
            default: {
                //意外状态错乱后，回到最初
                client->bodyLen = 0;//获取分包头后，得到的包长度
                if (client->pkgBuffer) {
                    free(client->pkgBuffer);
                    client->pkgBuffer = nullptr;//分包缓冲
                }
                client->index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
        }
    }
    cout << "Client_Interface " << __FUNCTION__ << " exit" << endl;

}

void FusionClient::ThreadProcessSend(void *p) {
    if (p == nullptr) {
        return;
    }
    auto client = (FusionClient *) p;

    uint8_t buf_send[1024 * 512] = {0};
    uint32_t len_send = 0;

    cout << "FusionClient " << __FUNCTION__ << " run" << endl;
    while (client->isRun) {
        //try lock_recv
        pthread_mutex_lock(&client->lock_send);
        while (client->queue_send.empty()) {
            pthread_cond_wait(&client->cond_send, &client->lock_send);
        }
        //task pop all msg
        while (!client->queue_send.empty()) {
            Pkg pkg = client->queue_send.front();
            client->queue_send.pop();
            bzero(buf_send, ARRAY_SIZE(buf_send));
            len_send = 0;

            //pack
            common::Pack(pkg, buf_send, &len_send);

            int ret = send(client->sockfd, buf_send, len_send, 0);
            if (ret != len_send) {
                if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                    continue;
                }
                cout << "msg:" << pkg.body << "send fail,errno:" << to_string((errno)) << endl;
                close(client->sockfd);
                timeval tv_now;
                gettimeofday(&tv_now, nullptr);
                cout << "close sock:" << to_string(client->sockfd) << ",at " << ctime((time_t *) &tv_now.tv_sec)
                     << endl;
                client->isRun = false;
            } else {
                cout << "msg:" << pkg.body << "send ok" << endl;
            }
        }
        pthread_mutex_unlock(&client->lock_send);
    }
    cout << "FusionClient " << __FUNCTION__ << " exit" << endl;

}

void FusionClient::ThreadCheckStatus(void *p) {
    if (p == nullptr) {
        return;
    }
    auto client = (FusionClient *) p;

    cout << "FusionClient " << __FUNCTION__ << " run" << endl;
    while (client->isRun) {
        sleep(client->checkStatus_timeval);

        timeval tv_now;

        gettimeofday(&tv_now, nullptr);
        if ((tv_now.tv_sec - client->receive_time.tv_sec) >= 2 * client->heartBeatTimeval) {
            //long time no receive
            client->isRun = false;
            cout << "long time no receive " << endl;
            cout << "now: " << ctime((time_t *) &tv_now.tv_sec) << endl;
            cout << "last receive: " << ctime((time_t *) &client->receive_time.tv_sec) << endl;
        }
    }
    cout << "FusionClient " << __FUNCTION__ << " exit" << endl;

}

int FusionClient::Send(Pkg pkg) {
    //try send lock_queue_recv
    pthread_mutex_lock(&this->lock_send);
    this->queue_send.push(pkg);
    pthread_cond_broadcast(&this->cond_send);
    pthread_mutex_unlock(&this->lock_send);

    return 0;
}

int FusionClient::SendToBase(const char *buf, int len) {
    int ret = send(sockfd, buf, len, 0);
    if (ret != len) {
        return -1;
    }

    return 0;
}

