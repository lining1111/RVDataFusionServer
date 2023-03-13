//
// Created by lining on 2023/3/13.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include "Nebulalink.h"
#include <glog/logging.h>

namespace Nebulalink {

    int RSU::CreateUdpSock() {
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
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

        return socketFd;
    }

    int RSU::Send(uint8_t *data, int len) {
        socklen_t addrLen = sizeof(struct sockaddr_in);

        int nleft = 0;
        int nsend = 0;
        if (sock) {
            std::unique_lock<std::mutex> lck(mtxSend);
            uint8_t *ptr = data;
            nleft = len;
            while (nleft > 0) {
                if ((nsend = sendto(sock, ptr, nleft, 0, (struct sockaddr *) &server_addr, addrLen)) < 0) {
                    if (errno == EINTR) {
                        nsend = 0;          /* and call send() again */
                    } else {
                        printf("消息data='%s' nsend=%d, 错误代码是%d\n", data, nsend, errno);
                        lck.unlock();
                        return (-1);
                    }
                } else if (nsend == 0) {
                    break;                  /* EOF */
                }
                nleft -= nsend;
                ptr += nsend;
            }
            lck.unlock();
        }

        return nsend;
    }

    int RSU::ThreadRecv(void *p) {
        if (p == nullptr) {
            LOG(ERROR) << "p null";
        }
        RSU *local = (RSU *) p;
        socklen_t sockaddrLen = sizeof(struct sockaddr_in);
        uint8_t *msg = new uint8_t(1024 * 1024);
        LOG(INFO) << "thread recv start";
        while (local->isRun) {
            usleep(1000 * 10);
            if (local->sock > 0 && local->rb != nullptr) {
                int len = recvfrom(local->sock, msg, (1024 * 1024), 0, (struct sockaddr *) &local->server_addr,
                                   &sockaddrLen);
                if (len > 0) {
                    local->rb->Write(msg, len);
                }
            }
        }
        delete[]msg;
        LOG(INFO) << "thread recv end";
        return 0;
    }

    int RSU::ThreadPRecv(void *p) {
        if (p == nullptr) {
            LOG(ERROR) << "p null";
        }
        RSU *local = (RSU *) p;
        socklen_t sockaddrLen = sizeof(struct sockaddr_in);
        LOG(INFO) << "thread precv start";
        while (local->isRun) {

        }

        LOG(INFO) << "thread precv end";
        return 0;
    }

    RSU::RSU(std::string ip) : ip(ip) {
        int fd = CreateUdpSock();
        if (fd > 0) {
            this->sock = fd;
            this->rb = new RingBuffer(RecvSize);
            this->isRun = true;
            futureRecv = std::async(ThreadRecv, this);
            futurePRecv = std::async(ThreadPRecv, this);
        }
    }

    RSU::~RSU() {
        if (this->isRun) {
            this->isRun = false;
            futureRecv.wait();
            futurePRecv.wait();
        }
        delete this->rb;
        if (this->sock > 0) {
            close(this->sock);
            this->sock = 0;
        }
    }

    bool RSU::IsRun() {
        return this->isRun;
    }

    int RSUCom::Frame::Pack(std::vector<uint8_t> &data) {
        data.clear();
        data.push_back(this->head.head[0]);
        data.push_back(this->head.head[1]);
        data.push_back(this->head.head[2]);
        data.push_back(this->head.head[3]);

        data.push_back(this->head.frameType);
        data.push_back(this->head.perceptionType);
        uint8_t lengthH = this->head.length >> 8;
        uint8_t lengthL = this->head.length;
        data.push_back(lengthL);
        data.push_back(lengthH);

        for (auto iter:this->data) {
            data.push_back(iter);
        }

        return 0;
    }

    int RSUCom::Frame::Unpack(std::vector<uint8_t> data) {
        if (data.size() < sizeof(Head)) {
            return -1;
        }
        int index = 0;
        memcpy(&this->head, data.data(), sizeof(this->head));
        index += sizeof(this->head);
        for (int i = index; i < data.size(); i++) {
            this->data.push_back(data.at(i));
        }
        return 0;
    }
}