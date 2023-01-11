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

ClientInfo::ClientInfo(int client_sock, struct sockaddr_in clientAddr, string name, void *super, int index,
                       long long int rbCapacity, timeval *readTimeout) :
        TcpServerClient(client_sock, clientAddr, name, super, index, rbCapacity, readTimeout) {
    this->type = 0;
    gettimeofday(&this->receive_time, nullptr);
    this->status = Start;

    this->needRelease.store(false);
    this->direction.store(Unknown);
    this->queuePkg.setMax(300);

//    timeval timeout;
//    timeout.tv_sec = 0;
//    timeout.tv_usec = 100 * 1000;
//    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
//    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    //创建以picsocknum为名的文件夹
//    dirName = "pic" + to_string(this->sock);
//    int isCreate = mkdir(dirName.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//    if (!isCreate)
//        Info("create path:%s\n", dirName.c_str());
//    else
//        Info("create path failed! error code : %s \n", isCreate, dirName.data());
}

ClientInfo::~ClientInfo() {
    TcpServerClient::Close();
    if (isThreadRun) {
        try {
            futureGetPkg.wait();
        } catch (std::future_error e) {
            Error(e.what());
        }
        try {
            futureGetPkgContent.wait();
        } catch (std::future_error e) {
            Error(e.what());
        }
    }

    delete[] this->pkgBuffer;
}


int ClientInfo::Run() {

    isThreadRun = true;

    //getPkg
    futureGetPkg = std::async(std::launch::async, ThreadGetPkg, this);
    //getPkgContent
    futureGetPkgContent = std::async(std::launch::async, ThreadGetPkgContent, this);

    return 0;
}

int ClientInfo::ThreadGetPkg(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return -1;
    }

    auto client = (ClientInfo *) pClientInfo;

    Notice("client-%d ip:%s %s run", client->sock, inet_ntoa(client->addr.sin_addr), __FUNCTION__);
    while (client->isConnect) {

        usleep(10);

        if (client->rb == nullptr || client->rb->GetReadLen() == 0) {
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
                    Error("CRC fail, 计算值:%d,包内值:%d", crc, pkg.crc.data);
                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        delete[] client->pkgBuffer;
                    }
                    client->index = 0;//分包缓冲的索引
                    client->status = Start;
                } else {

                    //记录接包时间
                    gettimeofday(&client->receive_time, nullptr);
                    //存入分包队列
                    if (!client->queuePkg.push(pkg)) {
//                        Info("client:%d 分包队列已满，丢弃此包", client->sock);
                    } else {
//                        Info("client:%d 分包队列存入", client->sock);
                    }
//                    Info("pkg queue size:%d", client->queuePkg.size());

                    client->bodyLen = 0;//获取分包头后，得到的包长度
                    if (client->pkgBuffer) {
                        delete[] client->pkgBuffer;
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
                    delete[] client->pkgBuffer;
                }
                client->index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
        }
    }
    Notice("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->addr.sin_addr), __FUNCTION__);
    return 0;
}

int ClientInfo::ThreadGetPkgContent(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return -1;
    }

    auto client = (ClientInfo *) pClientInfo;

    Notice("client-%d ip:%s %s run", client->sock, inet_ntoa(client->addr.sin_addr), __FUNCTION__);
    while (client->isConnect) {
        Pkg pkg;
        if (!client->queuePkg.pop(pkg)) {
            continue;
        }
//        Info("pkg cmd:%d", pkg.head.cmd);
        Json::Reader reader;
        Json::Value in;
        //解析分包，按照方法名不同，存入不同的队列
        switch (pkg.head.cmd) {
            case CmdType::CmdResponse : {
//                Debug("client-%d,应答指令", client->sock);
            }
                break;
            case CmdType::CmdLogin : {
//                Debug("client-%d,登录指令", client->sock);
            }
                break;
            case CmdType::CmdHeartBeat : {
//                Debug("client-%d,心跳指令", client->sock);
            }
                break;
            case CmdType::CmdFusionData : {
//                Debug("client-%d ip:%s,WatchData", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                //打印下接收的内容
//                Debug("%s\n", pkg.body.c_str());
//                Json::Reader reader;
//                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("watchData json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }

                WatchData watchData;
                watchData.JsonUnmarshal(in);

                Debug("client-%d ip:%s,timestamp:%f,imag:%d,obj size:%zu", client->sock,
                      inet_ntoa(client->addr.sin_addr),
                      watchData.timstamp,
                      watchData.isHasImage, watchData.lstObjTarget.size());

                //记录接包时间
                gettimeofday(&client->receive_time, nullptr);

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
                            Info("client:%d WatchData队列已满,丢弃消息", client->sock);
                        } else {
//                    Info("client:%d,timestamp %f WatchData存入", client->sock, watchData.timstamp);
                        }
//                        Info("client-%d WatchData size:%d", client->sock,
//                             server->dataUnitFusionData.i_queue_vector.at(i).size());
                    }
                }
            }
                break;
            case CmdType::CmdCrossTrafficJamAlarm : {
//                Debug("client-%d ip:%s,CrossTrafficJamAlarm", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
//                Json::Reader reader;
//                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("CrossTrafficJamAlarm json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                CrossTrafficJamAlarm crossTrafficJamAlarm;
                crossTrafficJamAlarm.JsonUnmarshal(in);
//                Debug("CrossTrafficJamAlarm client-%d ip:%s,timestamp:%f", client->sock,
//                      inet_ntoa(client->clientAddr.sin_addr),
//                      crossTrafficJamAlarm.timestamp);
                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitCrossTrafficJamAlarm.pushI(crossTrafficJamAlarm, client->indexSuper)) {
                    Info("client:%d %s CrossTrafficJamAlarm,丢弃消息", client->sock,
                         inet_ntoa(client->addr.sin_addr));
                } else {
//                    Info("client:%d,timestamp %f CrossTrafficJamAlarm存入", client->sock, crossTrafficJamAlarm.timestamp);
                }
//                Info("client-%d CrossTrafficJamAlarm size:%d", client->sock,
//                     server->dataUnitCrossTrafficJamAlarm.i_queue_vector.at(client->indexSuper).size());
            }
                break;
            case CmdType::CmdLineupInfoGather : {
//                Debug("client-%d ip:%s,LineupInfo", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
//                Json::Reader reader;
//                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("LineupInfo json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                LineupInfo lineupInfo;
                lineupInfo.JsonUnmarshal(in);

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitLineupInfoGather.pushI(lineupInfo, client->indexSuper)) {
                    Info("client:%d %s LineupInfo,丢弃消息", client->sock, inet_ntoa(client->addr.sin_addr));
                }
            }
                break;
            case CmdType::CmdPicData : {
//                Info("client-%d,设备视频数据回传", client->sock);
            }
                break;
            case CmdType::CmdTrafficFlowGather : {
//                Debug("client-%d ip:%s,TrafficFlow", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
//                Json::Reader reader;
//                Json::Value in;
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
                if (!server->dataUnitTrafficFlowGather.pushI(trafficFlow, client->indexSuper)) {
                    Info("client:%d  %s TrafficFlow队列已满,丢弃消息", client->sock, inet_ntoa(client->addr.sin_addr));
                }

            }
                break;
            case CmdType::CmdCarTrackGather : {
//                Debug("client-%d ip:%s,MultiViewCarTrack", client->sock, inet_ntoa(client->clientAddr.sin_addr));
                //打印下接收的内容
//                Info("%s\n", pkg.body.c_str());
//                Json::Reader reader;
//                Json::Value in;
                if (!reader.parse(pkg.body, in, false)) {
                    Error("MultiviewCarTrack json 解析失败%s", reader.getFormattedErrorMessages().c_str());
                    continue;
                }
                CarTrack multiViewCarTrack;
                multiViewCarTrack.JsonUnmarshal(in);

                //存入队列
                auto server = (FusionServer *) client->super;
                if (!server->dataUnitCarTrackGather.pushI(multiViewCarTrack, client->indexSuper)) {
                    Info("client:%d %s MultiViewCarTrack,丢弃消息", client->sock, inet_ntoa(client->addr.sin_addr));
                }
            }
                break;
            default: {
                //最后没有对应的方法名
                Error("client:%d %s 最后没有对应的方法名:%d,内容:%s", client->sock, inet_ntoa(client->addr.sin_addr),
                      pkg.head.cmd, pkg.body.c_str());
            }
                break;;
        }

    }
    Notice("client-%d ip:%s %s exit", client->sock, inet_ntoa(client->addr.sin_addr), __FUNCTION__);
    return 0;
}
