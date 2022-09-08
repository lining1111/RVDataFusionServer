//
// Created by lining on 2/15/22.
//

#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include "multiView/MultiViewClientInfo.h"
#include "log/Log.h"
#include "common/CRC.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

using namespace z_log;
using namespace common;

//string dirName;

MultiViewClientInfo::MultiViewClientInfo(struct sockaddr_in clientAddr, int client_sock, long long int rbCapacity) {
    this->msgid = 0;
    this->clientAddr = clientAddr;
    this->sock = client_sock;
    this->type = 0;
    bzero(this->extraData, sizeof(this->extraData) / sizeof(this->extraData[0]));
    gettimeofday(&this->receive_time, nullptr);
    this->rb = RingBuffer_New(rbCapacity);
    this->status = Start;

    this->isLive.store(false);
    this->isThreadDumpRun.store(false);
    this->isThreadGetPkgRun.store(false);
    this->isThreadGetPkgContentRun.store(false);
    this->needRelease.store(false);
    this->direction.store(Unknown);

    //创建以picsocknum为名的文件夹
//    dirName = "pic" + to_string(this->sock);
//    int isCreate = mkdir(dirName.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//    if (!isCreate)
//        Info("create path:%s\n", dirName.c_str());
//    else
//        Info("create path failed! error code : %s \n", isCreate, dirName.data());
}

MultiViewClientInfo::~MultiViewClientInfo() {

    if (isLive.load() == true) {
        isLive.store(false);
    }

    //等待所有线程退出
    do {
        sleep(1);
        cout << "等待所有线程退出" << endl;
    } while (isThreadDumpRun.load() || isThreadGetPkgRun.load() || isThreadGetPkgContentRun.load());

    if (sock > 0) {
        cout << "关闭sock：" << to_string(sock) << endl;
        close(sock);
    }

    if (pkgBuffer != nullptr) {
        free(pkgBuffer);
        pkgBuffer = nullptr;
    }

    RingBuffer_Delete(this->rb);
    this->rb = nullptr;
}

int MultiViewClientInfo::ProcessRecv() {
    isLive.store(true);

    //dump
    threadDump = thread(ThreadDump, this);
    string name = "ClientDump" + to_string(this->sock);
    pthread_setname_np(threadDump.native_handle(), name.data());
    threadDump.detach();
    //getPkg
    threadGetPkg = thread(ThreadGetPkg, this);
    string name1 = "ClientGetPkg" + to_string(this->sock);
    pthread_setname_np(threadGetPkg.native_handle(), name1.data());
    threadGetPkg.detach();
    //getPkgContent
    threadGetPkgContent = thread(ThreadGetPkgContent, this);
    string name2 = "ClientGetPkgContent" + to_string(this->sock);
    pthread_setname_np(threadGetPkgContent.native_handle(), name2.data());
    threadGetPkgContent.detach();

    return 0;
}

void MultiViewClientInfo::ThreadDump(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (MultiViewClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
    client->isThreadDumpRun.store(true);
    while (client->isLive.load()) {
        uint8_t buffer[1024 * 32];
        int len = 0;
        usleep(10);
        if (client->sock <= 0) {
            continue;
        }
//        timeval begin;
//        timeval end;
//
//        gettimeofday(&begin, nullptr);

        len = recv(client->sock, buffer, ARRAY_SIZE(buffer), 0);
        if ((len == -1) && (errno != EAGAIN) && (errno != EBUSY)) {
            Error("recv sock %d err:%s", client->sock, strerror(errno));
            //向服务端抛出应该关闭
//            client->needRelease.store(true);
        } else if (len > 0) {
            //将数据存入缓存
            if (client->rb != nullptr) {
//                timeval begin1;
//                gettimeofday(&begin1, nullptr);

                RingBuffer_Write(client->rb, buffer, len);
//
//                gettimeofday(&end, nullptr);
//
//                uint64_t cost1 = (end.tv_sec - begin1.tv_sec) * 1000 * 1000 + (end.tv_usec - begin1.tv_usec);
//                Info("client %d write buffer %d 耗时%lu us\n", client->sock, len, cost1);
//
//                uint64_t cost = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
//                Info("client %d recv %d 耗时%lu us\n", client->sock, len, cost);

            }
        }
    }
    client->isThreadDumpRun.store(false);
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}

void MultiViewClientInfo::ThreadGetPkg(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (MultiViewClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
    client->isThreadGetPkgRun.store(true);
    while (client->isLive.load()) {

        usleep(10);

        if (client->rb == nullptr || client->rb->available_for_read == 0) {
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
                    if (client->queuePkg.size() >= client->maxQueuePkg) {
//                        Info("client:%d 分包队列已满，丢弃此包:%d-%s", client->sock, pkg.head.cmd, pkg.body.c_str());
                        Info("client:%d 分包队列已满，丢弃此包", client->sock);
                    } else {
                        pthread_mutex_lock(&client->lockPkg);
                        //存入队列
                        client->queuePkg.push(pkg);
                        pthread_cond_broadcast(&client->condPkg);
                        pthread_mutex_unlock(&client->lockPkg);
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
    client->isThreadGetPkgRun.store(false);
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}

void MultiViewClientInfo::ThreadGetPkgContent(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (MultiViewClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
    client->isThreadGetPkgContentRun.store(true);
    while (client->isLive.load()) {
        usleep(10);
        if (client->queuePkg.size() == 0) {
            //分别队列为空
            continue;
        }
        Pkg pkg;

        pthread_mutex_lock(&client->lockPkg);
        while (client->queuePkg.size() == 0) {
            pthread_cond_wait(&client->condPkg, &client->lockPkg);
        }
        pkg = client->queuePkg.front();
        client->queuePkg.pop();
        pthread_cond_broadcast(&client->condPkg);
        pthread_mutex_unlock(&client->lockPkg);

        //解析分包，按照方法名不同，存入不同的队列
        switch (pkg.head.cmd) {
            case CmdType::Response : {
                Info("client-%d,应答指令", client->sock);
            }
                break;
            case CmdType::Login : {
                Info("client-%d,登录指令", client->sock);
            }
                break;
            case CmdType::HeartBeat : {
                Info("client-%d,心跳指令", client->sock);
            }
                break;
            case CmdType::DeviceMultiView : {
                Info("多目数据指令");
                //"WatchData"
                TrafficFlow trafficFlow;
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
                if (JsonUnmarshalTrafficFlow(pkg.body, trafficFlow) != 0) {
                    Error("trafficFlow json 解析失败");
                    continue;
                }
                Info("trafficFlow client-%d,timestamp:%f,flowData size:%d", client->sock, trafficFlow.timstamp,
                     trafficFlow.flowData.size());

                //根据结构体内的方向变量设置客户端的方向
//                client->direction.store(watchData.direction);
                //存下接收的json
                if (0) {
                    string saveDir = "recvJsonRoad" + trafficFlow.crossCode;
                    if (access(saveDir.c_str(), 0) == -1) {
                        cout << "dir:" << saveDir << " not exist,create" << endl;
                        mkdir(saveDir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                    }
                    //save file
                    string saveFile = saveDir + "/" + to_string(int(trafficFlow.timstamp)) + ".json";
                    ofstream outfile(saveFile, ios::trunc);
                    outfile << pkg.body;
                    outfile.flush();
                    outfile.close();
                }

                //存入队列
                if (client->queueTrafficFlow.size() >= client->maxQueueTrafficFlow) {
//                    Info("client:%d TrafficFlow队列已满,丢弃消息:%d-%s", client->sock, pkg.head.cmd, pkg.body.c_str());
                    Info("client:%d TrafficFlow队列已满,丢弃消息", client->sock);
                } else {
                    pthread_mutex_lock(&client->lockTrafficFlow);
                    //存入队列
                    client->queueTrafficFlow.push(trafficFlow);
                    pthread_cond_broadcast(&client->condTrafficFlow);
                    pthread_mutex_unlock(&client->lockTrafficFlow);
//                    Info("client:%d TrafficFlow队列存入消息:%d-%s", client->sock, pkg.head.cmd, pkg.body.c_str());
                }

            }
                break;
            case CmdType::DeviceAlarm : {
                Info("client-%d,设备报警指令", client->sock);
            }
                break;
            case CmdType::DeviceStatus : {
                Info("client-%d,设备状态指令", client->sock);
            }
                break;
            case CmdType::DevicePicData : {
                Info("client-%d,设备视频数据回传", client->sock);
            }
                break;
            default: {
                //最后没有对应的方法名
                Info("client:%d 最后没有对应的方法名:%d,内容:%s", client->sock, pkg.head.cmd,
                     pkg.body.c_str());
            }
                break;;
        }

    }
    client->isThreadGetPkgContentRun.store(false);
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}
