//
// Created by lining on 2/15/22.
//

#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "server/ClientInfo.h"
#include "server/FusionServer.h"
#include "log/Log.h"
#include "common/CRC.h"

using namespace z_log;
using namespace common;

//string dirName;

ClientInfo::ClientInfo(struct sockaddr_in clientAddr, int client_sock, void *super, int index,
                       long long int rbCapacity) {
    this->super = super;
    this->indexSuper = index;
    this->clientAddr = clientAddr;
    this->sock = client_sock;
    this->type = 0;
    bzero(this->extraData, sizeof(this->extraData) / sizeof(this->extraData[0]));
    pthread_mutex_lock(&lock_receive_time);
    gettimeofday(&this->receive_time, nullptr);
    pthread_mutex_unlock(&lock_receive_time);
    this->rb = RingBuffer_New(rbCapacity);
    this->status = Start;

    this->isLive.store(false);
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

ClientInfo::~ClientInfo() {
    Close();
}

int ClientInfo::Close() {
    if (isLive.load() == true) {
        isLive.store(false);
    }
//    sleep(1);
    if (sock > 0) {
        Info("关闭sock：%d", sock);
        close(sock);
    }

    if (pkgBuffer != nullptr) {
        free(pkgBuffer);
        pkgBuffer = nullptr;
    }

    RingBuffer_Delete(this->rb);
    this->rb = nullptr;
    return 0;
}

int ClientInfo::ProcessRecv() {
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

void ClientInfo::ThreadDump(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
    while (client->isLive.load()) {
        uint8_t buffer[1024 * 1024];
        int len = 0;
//        usleep(10);
        if (client->sock <= 0) {
            continue;
        }
//        Info("sock %d recv begin========", client->sock);
        len = recv(client->sock, buffer, ARRAY_SIZE(buffer), 0);
//        Info("sock %d recv end========", client->sock);
        if ((len == -1) && (errno != EAGAIN) && (errno != EBUSY) && (errno != EWOULDBLOCK)) {
            Error("recv sock %d err:%s", client->sock, strerror(errno));
            //向服务端抛出应该关闭
//            client->needRelease.store(true);
        } else if (len > 0) {
            //将数据存入缓存
            if (client->rb != nullptr) {
                RingBuffer_Write(client->rb, buffer, len);

                pthread_mutex_lock(&client->lock_receive_time);
                client->isReceive_timeSet = true;
                gettimeofday(&client->receive_time, nullptr);
                pthread_mutex_unlock(&client->lock_receive_time);
            }
        }
    }
    //这里退出的打印可能不正常，可以sock都关闭了
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}

void ClientInfo::ThreadGetPkg(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
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
                    pthread_mutex_lock(&client->lock_receive_time);
                    gettimeofday(&client->receive_time, nullptr);
                    pthread_mutex_unlock(&client->lock_receive_time);

                    //存入分包队列
                    if (!client->queuePkg.push(pkg)) {
//                        Info("client:%d 分包队列已满，丢弃此包", client->sock);
                    } else {
//                        Info("client:%d 分包队列存入", client->sock);
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
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}

void ClientInfo::ThreadGetPkgContent(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("client-%d ip:%s %s run", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);
    while (client->isLive.load()) {
        Pkg pkg;
        if (!client->queuePkg.pop(pkg)) {
            continue;
        }

        //解析分包，按照方法名不同，存入不同的队列
        switch (pkg.head.cmd) {
            case CmdType::Response : {
                Debug("client-%d,应答指令", client->sock);
            }
                break;
            case CmdType::Login : {
                Debug("client-%d,登录指令", client->sock);
            }
                break;
            case CmdType::HeartBeat : {
                Debug("client-%d,心跳指令", client->sock);
            }
                break;
            case CmdType::DeviceData : {
                //打印下接收的内容
//                Debug("%s\n", pkg.body.c_str());
                Json::Reader reader;
                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("watchData json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }

                WatchData watchData;
                watchData.JsonUnmarshal(pkg.body);

                Debug("client-%d ip:%s,timestamp:%f,imag:%d,obj size:%zu", client->sock,
                     inet_ntoa(client->clientAddr.sin_addr),
                     watchData.timstamp,
                     watchData.isHasImage, watchData.lstObjTarget.size());

                //记录接包时间
                pthread_mutex_lock(&client->lock_receive_time);
                gettimeofday(&client->receive_time, nullptr);
                pthread_mutex_unlock(&client->lock_receive_time);


                //根据结构体内的方向变量设置客户端的方向
                client->direction.store(watchData.direction);

                //按照方向顺序放入
                auto server = (FusionServer *) client->super;
                for (int i = 0; i < ARRAY_SIZE(server->roadDirection); i++) {
                    if (server->roadDirection[i] == client->direction) {
                        //方向相同，放入对应索引下数组
                        //存入队列
                        if (!server->dataUnitFusionData.pushI(watchData, i)) {
//                    Info("client:%d WatchData队列已满,丢弃消息:%d-%s", client->sock, pkg.head.cmd, pkg.body.c_str());
//                    Info("client:%d WatchData队列已满,丢弃消息", client->sock);
                        } else {
//                    Info("client:%d,timestamp %f WatchData存入", client->sock, watchData.timstamp);
                        }
                    }
                }

            }
                break;
            case CmdType::DeviceMultiview : {
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
                Json::Reader reader;
                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("TrafficFlow json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                TrafficFlow trafficFlow;
                trafficFlow.JsonUnmarshal(in);

//                Debug("trafficFlow client-%d,timestamp:%f,flowData size:%d", client->sock, trafficFlow.timstamp,
//                     trafficFlow.flowData.size());

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitTrafficFlows.pushI(trafficFlow, client->indexSuper)) {
                    Debug("client:%d  %s TrafficFlow队列已满,丢弃消息", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                }

            }
                break;
            case CmdType::DeviceAlarm : {
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
                Json::Reader reader;
                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("CrossTrafficJamAlarm json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                CrossTrafficJamAlarm crossTrafficJamAlarm;
                crossTrafficJamAlarm.JsonUnmarshal(in);

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitCrossTrafficJamAlarm.pushI(crossTrafficJamAlarm, client->indexSuper)) {
                    Debug("client:%d %s CrossTrafficJamAlarm,丢弃消息", client->sock,
                         inet_ntoa(client->clientAddr.sin_addr));
                }
            }
                break;
            case CmdType::DeviceStatus : {
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
                Json::Reader reader;
                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("LineupInfo json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                LineupInfo lineupInfo;
                lineupInfo.JsonUnmarshal(in);

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitLineupInfoGather.pushI(lineupInfo, client->indexSuper)) {
                    Debug("client:%d %s LineupInfo,丢弃消息", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                }
            }
                break;
            case CmdType::DevicePicData : {
                Info("client-%d,设备视频数据回传", client->sock);
            }
                break;
            case CmdType::DeviceMultiviewCarTrack : {
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
                Json::Reader reader;
                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("MultiviewCarTrack json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                MultiViewCarTrack multiViewCarTrack;
                multiViewCarTrack.JsonUnmarshal(in);

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitMultiViewCarTracks.pushI(multiViewCarTrack, client->indexSuper)) {
                    Debug("client:%d %s MultiViewCarTrack,丢弃消息", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                }
            }
                break;
            default: {
                //最后没有对应的方法名
                Debug("client:%d %s 最后没有对应的方法名:%d,内容:%s", client->sock, inet_ntoa(client->clientAddr.sin_addr),
                     pkg.head.cmd, pkg.body.c_str());
            }
                break;;
        }

    }
    Info("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->clientAddr.sin_addr), __FUNCTION__);

}
