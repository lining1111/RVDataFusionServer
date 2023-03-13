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
                int readLen = (1024 * 1024 < local->rb->GetWriteLen() ? (1024 * 1024) : local->rb->GetWriteLen());

                int len = recvfrom(local->sock, msg, readLen, 0, (struct sockaddr *) &local->server_addr,
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

    static bool isTag(std::vector<uint8_t> data) {
        if (data.size() < TagLen) {
            return false;
        }

        if ((data.at(0) == 0xda) && (data.at(1) == 0xdb) && (data.at(2) == 0xdc) && (data.at(3) == 0xdd)) {
            return true;
        } else {
            return false;
        }
    }

    void tagForward(std::vector<uint8_t> &data) {
        std::vector<uint8_t> tmp;
        for (int i = 1; i < data.size(); i++) {
            tmp.push_back(data.at(i));
        }
        data.assign(tmp.begin(), tmp.end());
    }


    int RSU::ThreadPRecv(void *p) {
        if (p == nullptr) {
            LOG(ERROR) << "p null";
        }
        RSU *local = (RSU *) p;
        socklen_t sockaddrLen = sizeof(struct sockaddr_in);
        LOG(INFO) << "thread precv start";
        while (local->isRun && local->rb != nullptr) {
            usleep(1000 * 10);
            switch (local->recvState) {
                case SStart: {
                    //找到tag
                    if (local->rb->GetReadLen() > 1) {
                        uint8_t val;
                        if (local->rb->Read(&val, 1)) {
                            local->tag.push_back(val);
                        }

                        if (local->tag.size() >= TagLen) {
                            //判断是否全对
                            if (isTag(local->tag)) {
                                for (int i = 0; i < TagLen; i++) {
                                    local->frametmp.head.head[i] = local->tag.at(i);
                                }
                                local->tag.clear();
                                local->recvState = SHead;
                            } else {
                                tagForward(local->tag);
                            }
                        }
                    }
                }
                    break;
                case SHead: {
                    //找到全部头
                    if (local->rb->GetReadLen() > (sizeof(RSUCom::Head) - TagLen)) {
                        uint8_t *left = new uint8_t[sizeof(RSUCom::Head) - TagLen];
                        if (local->rb->Read(left, sizeof(RSUCom::Head) - TagLen)) {
                            memcpy(&local->frametmp.head.frameType, left, (sizeof(RSUCom::Head) - TagLen));
                            local->recvState = SData;
                        }
                        delete[] left;
                    }
                }
                    break;
                case SData: {
                    //找到全部数据
                    if (local->rb->GetReadLen() > local->frametmp.head.length) {
                        for (int i = 0; i < local->frametmp.head.length; i++) {
                            uint8_t val;
                            if (local->rb->Read(&val, 1)) {
                                local->frametmp.data.push_back(val);
                            }
                        }
                        local->recvState = SEnd;
                    }
                }
                    break;
                case SEnd: {
                    local->frames.push(local->frametmp);
                    local->recvState = SStart;
                }
                    break;
            }
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
            frames.setMax(30);
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

    static int PFrameUnknown(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameRSU(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameV2XBroadcast(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameCamera(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameMRadar(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameMC(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameLRadar(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    static int PFrameMEC(RSU *local, std::vector<uint8_t> data) {
        if (local == nullptr) {
            return -1;
        }
        return 0;
    }

    typedef int (*PFrameFun)(RSU *local, std::vector<uint8_t> data);

    static std::map<RSUCom::PerceptionType, PFrameFun> PFrameMap = {
            make_pair(RSUCom::PerceptionType_Unknown, PFrameUnknown),
            make_pair(RSUCom::PerceptionType_RSU, PFrameRSU),
            make_pair(RSUCom::PerceptionType_V2XBroadcast, PFrameV2XBroadcast),
            make_pair(RSUCom::PerceptionType_Camera, PFrameCamera),
            make_pair(RSUCom::PerceptionType_MRadar, PFrameMRadar),
            make_pair(RSUCom::PerceptionType_MC, PFrameMC),
            make_pair(RSUCom::PerceptionType_LRadar, PFrameLRadar),
            make_pair(RSUCom::PerceptionType_MEC, PFrameMEC),
    };


    int RSU::ThreadPFrame(void *p) {
        if (p == nullptr) {
            LOG(ERROR) << "p null";
        }
        RSU *local = (RSU *) p;
        LOG(INFO) << "thread pframe start";
        while (local->isRun) {
            RSUCom::Frame frame;
            if (local->frames.pop(frame)) {
                //按照cmd分别处理
                auto iter = PFrameMap.find(RSUCom::PerceptionType(frame.head.perceptionType));
                if (iter != PFrameMap.end()) {
                    iter->second(local, frame.data);
                } else {
                    //最后没有对应的方法名
                    VLOG(2) << "client:" << inet_ntoa(local->server_addr.sin_addr)
                            << " 最后没有对应的方法名:" << frame.head.perceptionType;
                }
            }
        }
        LOG(INFO) << "thread pframe end";
        return 0;
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