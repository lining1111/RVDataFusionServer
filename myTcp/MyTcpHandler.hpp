//
// Created by lining on 2023/8/22.
//

#ifndef RVDATAFUSIONSERVER_MYTCPHANDLER_H
#define RVDATAFUSIONSERVER_MYTCPHANDLER_H

#include "common/common.h"
#include "common/CRC.h"
#include "common/proc.h"
#include "common/config.h"
#include "ringbuffer/RingBuffer.h"
#include "Queue.hpp"
#include <glog/logging.h>
#include <thread>
#include <vector>
#include <string>

using namespace std;


class MyTcpHandler {
public:
    std::string _peerAddress;
    enum {
        BUFFER_SIZE = 1024 * 1024 * 64
    };
    bool _isRun = false;
    RingBuffer *rb = nullptr;
    Queue<common::Pkg> pkgs;
    //接收buffer状态机 Start开始---从接收的数据中寻找帧头开始(开始为特殊字符$)GetHeadStart---找到帧头开始，获取全部帧头信息(一共9字节)GetHead---
    //获取全部帧头信息后，根据帧头信息的帧内容长度信息，获取全部帧内容GetBody---获取完身体后，获取CRC GETCRC---获取完全部帧内容后，回到开始状态Start
    typedef enum {
        Start = 0, GetHeadStart, GetHead, GetBody, GetCRC,
    } RecvStatus;//表示状态机已经达到的状态
    RecvStatus status = Start;
    //用于缓存解包
    common::PkgHead pkgHead;//分包头
    int bodyLen = 0;//获取分包头后，得到的包长度
    uint8_t *pkgBuffer = nullptr;
    int pkgBufferIndex = 0;

    MyTcpHandler() {
        rb = new RingBuffer(BUFFER_SIZE);
        startBussness();
    }

    ~MyTcpHandler() {
        _isRun = false;
        if (rb != nullptr) {
            delete rb;
            rb = nullptr;
        }
    }

    void startBussness() {
        _isRun = true;
        thread tsm(ThreadStateMachine, this);
        tsm.detach();
        thread tpp(ThreadProcessPkg, this);
        tpp.detach();
    }

    static void ThreadStateMachine(MyTcpHandler *local) {
        LOG(INFO) << " ThreadStateMachine start";
        while (local->_isRun) {
            usleep(10);
            if (local->rb == nullptr) {
                //数据缓存区不存在
                continue;
            }
            if (local->rb->GetReadLen() == 0) {
                continue;
            }

            //分包状态机
            switch (local->status) {
                case Start: {
                    //开始,寻找包头的开始标志
                    if (local->rb->GetReadLen() >= MEMBER_SIZE(PkgHead, tag)) {
                        char start = '\0';
                        local->rb->Read(&start, sizeof(start));
                        if (start == '$') {
                            //找到开始标志
                            local->status = GetHeadStart;
                        }
                    }
                }
                    break;
                case GetHeadStart: {
                    //开始寻找头
                    if (local->rb->GetReadLen() >= (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag))) {
                        //获取完整的包头
                        local->pkgHead.tag = '$';
                        local->rb->Read(&local->pkgHead.version, (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag)));
                        //buffer申请内存
                        local->pkgBuffer = new uint8_t[local->pkgHead.len];
                        local->pkgBufferIndex = 0;
                        memcpy(local->pkgBuffer, &local->pkgHead, sizeof(pkgHead));
                        local->pkgBufferIndex += sizeof(pkgHead);
                        local->status = GetHead;
                        local->bodyLen = local->pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                    }
                }
                    break;
                case GetHead: {
                    if (local->bodyLen <= 0) {
                        local->bodyLen = local->pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                    }
                    if (local->rb->GetReadLen() >= local->bodyLen) {
                        //获取正文
                        local->rb->Read(local->pkgBuffer + local->pkgBufferIndex, local->bodyLen);

                        local->pkgBufferIndex += local->bodyLen;
                        local->status = GetBody;
                    }
                }
                    break;
                case GetBody: {
                    if (local->rb->GetReadLen() >= sizeof(PkgCRC)) {
                        //获取CRC
                        local->rb->Read(local->pkgBuffer + local->pkgBufferIndex, sizeof(PkgCRC));
                        local->pkgBufferIndex += sizeof(PkgCRC);
                        local->status = GetCRC;
                    }
                }
                    break;
                case GetCRC: {
                    //获取CRC 就获取全部的分包内容，转换为结构体
                    Pkg pkg;

                    Unpack(local->pkgBuffer, local->pkgHead.len, pkg);

                    //判断CRC是否正确
                    //打印下buffer
//                PrintHex(local->pkgBuffer, local->pkgHead.len);
                    uint16_t crc = Crc16TabCCITT(local->pkgBuffer, local->pkgHead.len - 2);
                    if (crc != pkg.crc.data) {//CRC校验失败
                        VLOG(2) << "CRC fail, 计算值:" << crc << ",包内值:%d" << pkg.crc.data;
                        local->bodyLen = 0;//获取分包头后，得到的包长度
                        if (local->pkgBuffer != nullptr) {
                            delete[] local->pkgBuffer;
                            local->pkgBuffer = nullptr;
                        }
                        local->pkgBufferIndex = 0;//分包缓冲的索引
                        local->status = Start;
                    } else {

                        //存入分包队列
                        if (!local->pkgs.push(pkg)) {

                        } else {

                        }
                        local->bodyLen = 0;//获取分包头后，得到的包长度
                        if (local->pkgBuffer != nullptr) {
                            delete[] local->pkgBuffer;
                            local->pkgBuffer = nullptr;
                        }
                        local->pkgBufferIndex = 0;//分包缓冲的索引
                        local->status = Start;
                    }
                }
                    break;
                default: {
                    //意外状态错乱后，回到最初
                    local->bodyLen = 0;//获取分包头后，得到的包长度
                    if (local->pkgBuffer != nullptr) {
                        delete[] local->pkgBuffer;
                        local->pkgBuffer = nullptr;
                    }
                    local->pkgBufferIndex = 0;//分包缓冲的索引
                    local->status = Start;
                }
                    break;
            }
        }
        LOG(INFO) << " ThreadStateMachine end";
    }

    static void ThreadProcessPkg(MyTcpHandler *local) {
        LOG(INFO) << " ThreadProcessPkg start";
        while (local->_isRun) {
            usleep(10);
            common::Pkg pkg;
            if (local->pkgs.pop(pkg)) {
                //按照cmd分别处理
                auto iter = PkgProcessMap.find(CmdType(pkg.head.cmd));
                if (iter != PkgProcessMap.end()) {
                    iter->second(local->_peerAddress, pkg.body);
                } else {
                    //最后没有对应的方法名
//                VLOG(2) << "client:" << inet_ntoa(client->addr.sin_addr)
//                        << " 最后没有对应的方法名:" << pkg.head.cmd << ",内容:" << pkg.body;
                }
            }
        }
        LOG(INFO) << " ThreadProcessPkg end";
    }

};


#endif //RVDATAFUSIONSERVER_MYTCPHANDLER_H
