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
#include <glog/logging.h>
#include "net/cpp-icmplib.h"

using namespace common;

FusionClient::FusionClient(string server_ip, unsigned int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
}

FusionClient::~FusionClient() {
    Close();
}

int FusionClient::Open() {
    if (this->server_ip.empty()) {
        LOG(ERROR) << "server ip empty" << std::endl;
        return -1;
    }
    //先ping下远端开是否可以连接必须在root权限下使用
    if (icmplib::Ping(server_ip,5).response != icmplib::PingResponseType::Success) {
//        LOG(ERROR) << "client not connect:" << server_ip;
        return -1;
    }


    int sockfd = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) {
        LOG(ERROR) << "create socket failed" << endl;
        return -1;
    } else {
        int opt = 1;
//        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;
//        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    }

    struct sockaddr_in server_addr;
    int ret = 0;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(this->server_ip.c_str());
    server_addr.sin_port = htons(this->server_port);

    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    timeval tv_now;
    gettimeofday(&tv_now, nullptr);

    if (ret == -1) {
        LOG(ERROR) << "connect server fail:" << this->server_ip << ":" << this->server_port;
        close(sockfd);
        return -1;
    }

    LOG(INFO) << "connect server success:" << this->server_ip << ":" << this->server_port;
    rb = new RingBuffer(RecvSize);
    this->sockfd = sockfd;
    isRun = true;
    return 0;
}

int FusionClient::Run() {
    if (!this->isRun) {
        LOG(ERROR) << "client not connect:" << server_ip << ":" << server_port;
        return -1;
    }

    isLocalThreadRun = true;
    //start pthread
    futureDump = std::async(std::launch::async, ThreadDump, this);
    futureProcessRev = std::async(std::launch::async, ThreadProcessRecv, this);
    futureProcessSend = std::async(std::launch::async, ThreadProcessSend, this);
    //因为上层不会回复，所以不检查状态
//    thread_checkStatus = thread(ThreadCheckStatus, this);
//    thread_checkStatus.detach();
    StartTimerTask();
    return 0;
}

void FusionClient::addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task) {
    Timer *timer = new Timer(name);
    timer->start(timeval_ms, task);
    timerTasks.insert(make_pair(name, timer));
}

void FusionClient::deleteTimerTask(string name) {
    auto timerTask = timerTasks.find(name);
    if (timerTask != timerTasks.end()) {
        delete timerTask->second;
        timerTasks.erase(timerTask++);
    }
}


void FusionClient::StartTimerTask() {

}

void FusionClient::StopTimerTaskAll() {
    auto iter = timerTasks.begin();
    while (iter != timerTasks.end()) {
        delete iter->second;
        iter = timerTasks.erase(iter);
    }
}

int FusionClient::Close() {
    StopTimerTaskAll();

    isRun = false;

    if (sockfd > 0) {
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }
    if (isLocalThreadRun) {
        try {
            futureDump.wait();
        } catch (std::future_error e) {
            LOG(ERROR) << e.what();
        }
        try {
            futureProcessRev.wait();
        } catch (std::future_error e) {
            LOG(ERROR) << e.what();
        }
        try {
            futureProcessSend.wait();
        } catch (std::future_error e) {
            LOG(ERROR) << e.what();
        }
    }

    delete rb;

    delete[] pkgBuffer;

    return 0;
}

int FusionClient::ThreadDump(void *p) {
    if (p == nullptr) {
        return -1;
    }
    auto client = (FusionClient *) p;

    char *buf = new char[1024 * 512];
    int nread = 0;

    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " run";
    while (client->isRun) {
        int recvLen = (client->rb->GetWriteLen() < (1024 * 512)) ? client->rb->GetWriteLen() : (1024 * 512);

        nread = recv(client->sockfd, buf, recvLen, 0);
        if (nread < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            LOG(ERROR) << "recv info error,errno:" << strerror(errno);
            close(client->sockfd);
            timeval tv_now;
            gettimeofday(&tv_now, nullptr);
            LOG(ERROR) << "close sock:" << to_string(client->sockfd) << endl;
            client->isRun = false;
        } else if (nread > 0) {
//            cout << "recv info" << endl;
            //update receive_time
            gettimeofday(&client->receive_time, nullptr);
            //把接受数据放到
            client->rb->Write(buf, nread);
        }
        usleep(10);
    }
    delete[] buf;
    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " exit";
    return 0;
}

int FusionClient::ThreadProcessRecv(void *p) {
    if (p == nullptr) {
        return -1;
    }
    auto client = (FusionClient *) p;

    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " run";
    while (client->isRun) {
        usleep(100);
        if (client->rb == nullptr) {
            //数据缓存区不存在
            continue;
        }

        //分包状态机
        switch (client->status) {
            case Start: {
                //开始,寻找包头的开始标志
                if (client->rb->GetReadLen() >= MEMBER_SIZE(PkgHead, tag)) {
                    char start = '\0';
                    client->rb->Read(&start, sizeof(start));
                    if (start == '$') {
                        //找到开始标志
                        client->status = GetHeadStart;
                    }
                }
            }
                break;
            case GetHeadStart: {
                //开始寻找头
                if (client->rb->GetReadLen() >= (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag))) {
                    //获取完整的包头
                    client->pkgHead.tag = '$';
                    client->rb->Read(&client->pkgHead.version, (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag)));
                    //buffer申请内存
                    client->pkgBuffer = new uint8_t[client->pkgHead.len];
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
                if (client->rb->GetReadLen() >= client->bodyLen) {
                    //获取正文
                    client->rb->Read(client->pkgBuffer + client->index, client->bodyLen);
                    client->index += client->bodyLen;
                    client->status = GetBody;
                }
            }
                break;
            case GetBody: {
                if (client->rb->GetReadLen() >= sizeof(PkgCRC)) {
                    //获取CRC
                    client->rb->Read(client->pkgBuffer + client->index, sizeof(PkgCRC));
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
                    VLOG(2) << "CRC fail, 计算值:" << crc << ",包内值:%d" << pkg.crc.data;
                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        delete[] client->pkgBuffer;
                        client->pkgBuffer = nullptr;//分包缓冲
                    }
                    client->index = 0;//分包缓冲的索引
                    client->status = Start;
                } else {

                    //记录接包时间
                    gettimeofday(&client->receive_time, nullptr);

                    //存入分包队列
                    if (!client->queuePkg.push(pkg)) {
//                        Info("client:%d 分包队列已满，丢弃此包:%d-%s", client->sockfd, pkg.head.cmd, pkg.body.c_str());
                    }

                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        delete[] client->pkgBuffer;
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
                    delete[] client->pkgBuffer;
                    client->pkgBuffer = nullptr;//分包缓冲
                }
                client->index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
        }
    }
    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " exit";
    return 0;
}

int FusionClient::ThreadProcessSend(void *p) {
    if (p == nullptr) {
        return -1;
    }
    auto client = (FusionClient *) p;

    uint8_t *buf_send = new uint8_t[1024 * 1024];
    uint32_t len_send = 0;

    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " run";
    while (client->isRun) {
        Pkg pkg;
        if (!client->queue_send.pop(pkg)) {
            continue;
        }
        pthread_mutex_lock(&client->lock_sock);
        uint32_t len_send = 0;
        //pack
        common::Pack(pkg, buf_send, &len_send);
        //尽力发送
        int nleft = 0;
        int nsend = 0;
        uint8_t *ptr = buf_send;
        nleft = len_send;
        while (nleft > 0) {
            if ((nsend = send(client->sockfd, ptr, nleft, 0)) < 0) {
                printf("消息 nsend=%d, 错误代码是%d\n", nsend, errno);
                client->isRun = false;
                break;
            } else if (nsend == 0) {
                client->isRun = false;
                break;                  /* EOF */
            }
            nleft -= nsend;
            ptr += nsend;
        }
        pthread_mutex_unlock(&client->lock_sock);
    }
    delete[] buf_send;
    LOG(INFO) << "FusionClient," << client->server_ip << ":" << client->server_port << " " << __FUNCTION__ << " exit";
    return 0;
}

int FusionClient::SendQueue(Pkg pkg) {
    //try send lock_queue_recv
    if (!queue_send.push(pkg)) {
        LOG(ERROR) << __FUNCTION__ << " fail";
        return -1;
    }
    return 0;
}

int FusionClient::SendBase(Pkg pkg) {
    int ret = 0;

    pthread_mutex_lock(&lock_sock);
    uint8_t *buf_send = new uint8_t[1024 * 1024];
    uint32_t len_send = 0;
    len_send = 0;
    //pack
    common::Pack(pkg, buf_send, &len_send);
    //尽力发送
    int nleft = 0;
    int nsend = 0;
    uint8_t *ptr = buf_send;
    nleft = len_send;
    while (nleft > 0) {
        if ((nsend = send(sockfd, ptr, nleft, 0)) < 0) {
            ret = -1;
            printf("消息 nsend=%d, 错误代码是%d\n", nsend, errno);
            isRun = false;
            break;
        } else if (nsend == 0) {
            ret = -1;
            isRun = false;
            break;                  /* EOF */
        }
        nleft -= nsend;
        ptr += nsend;
    }
    delete[] buf_send;
    pthread_mutex_unlock(&lock_sock);
    return ret;
}

