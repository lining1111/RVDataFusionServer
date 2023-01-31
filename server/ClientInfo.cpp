//
// Created by lining on 2/15/22.
//

#include <cstring>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "server/ClientInfo.h"
#include "server/FusionServer.h"
#include "glog/logging.h"
#include "common/CRC.h"

using namespace common;

ClientInfo::ClientInfo(int client_sock, struct sockaddr_in clientAddr, string name, void *super,
                       long long int rbCapacity, timeval *readTimeout) :
        TcpServerClient(client_sock, clientAddr, name, super, rbCapacity, readTimeout) {
    this->type = 0;
    gettimeofday(&this->receive_time, nullptr);
    this->status = Start;
    this->super = super;
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
            LOG(INFO) << e.what();
        }
        try {
            futureGetPkgContent.wait();
        } catch (std::future_error e) {
            LOG(INFO) << e.what();
        }
    }

    delete[] this->pkgBuffer;
}


int ClientInfo::Run() {

    TcpServerClient::Run();
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

    LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << __FUNCTION__ << " run";
    while (client->isConnect) {

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
                    LOG(INFO) << "CRC fail, 计算值:" << crc << ",包内值:%d" << pkg.crc.data;
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
                    delete[] client->pkgBuffer;
                }
                client->index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
        }
        usleep(10);
    }
    LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << __FUNCTION__ << " exit";
    return 0;
}

static int PkgProcessFun_CmdResponse(ClientInfo *client, string content) {
    LOG(INFO) << "应答指令";
    return 0;
}

static int PkgProcessFun_CmdLogin(ClientInfo *client, string content) {
    LOG(INFO) << "登录指令";
    return 0;
}

static int PkgProcessFun_CmdHeartBeat(ClientInfo *client, string content) {
    LOG(INFO) << "心跳指令";
    return 0;
}

static int PkgProcessFun_CmdFusionData(ClientInfo *client, string content) {
    int ret = 0;
//    Debug("client-%d ip:%s,WatchData", client->sock, inet_ntoa(client->addr.sin_addr));
    //打印下接收的内容
//    Debug("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "watchData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }

    WatchData watchData;
    watchData.JsonUnmarshal(in);

//    Debug("client-%d ip:%s,timestamp:%f,imag:%d,obj size:%d",
//          client->sock, inet_ntoa(client->addr.sin_addr), watchData.timstamp, watchData.isHasImage,
//          watchData.lstObjTarget.size());

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
//                Info("client:%d WatchData队列已满,丢弃消息:%s", client->sock, content.c_str());
                LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " FusionData,丢弃消息";
                ret = -1;
            }
//            Info("client-%d WatchData size:%d", client->sock, server->dataUnitFusionData.i_queue_vector.at(i).size());
            break;
        }
    }
    return ret;
}

static int PkgProcessFun_CmdCrossTrafficJamAlarm(ClientInfo *client, string content) {
    int ret = 0;
//    Debug("client-%d ip:%s,CrossTrafficJamAlarm", client->sock, inet_ntoa(client->addr.sin_addr));
    //打印下接收的内容
//    Info("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "CrossTrafficJamAlarm json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    CrossTrafficJamAlarm crossTrafficJamAlarm;
    crossTrafficJamAlarm.JsonUnmarshal(in);
//    Debug("CrossTrafficJamAlarm client-%d ip:%s,timestamp:%f",
//          client->sock, inet_ntoa(client->addr.sin_addr), crossTrafficJamAlarm.timestamp);
    //存入队列
    auto server = (FusionServer *) client->super;

    if (!server->dataUnitCrossTrafficJamAlarm.pushI(crossTrafficJamAlarm, server->FindIndexInUnOrder(
            crossTrafficJamAlarm.hardCode))) {
        LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " CrossTrafficJamAlarm,丢弃消息";
        ret = -1;
    }
//    Info("client-%d CrossTrafficJamAlarm timestamp:%f", client->sock, crossTrafficJamAlarm.timestamp);
    return ret;
}

static int PkgProcessFun_CmdIntersectionOverflowAlarm(ClientInfo *client, string content) {
    int ret = 0;
//    Info("client-%d,IntersectionOverflowAlarm", client->sock);
    //打印下接收的内容
//    Info("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "IntersectionOverflowAlarm json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    IntersectionOverflowAlarm intersectionOverflowAlarm;
    intersectionOverflowAlarm.JsonUnmarshal(in);
//    Debug("CrossTrafficJamAlarm client-%d ip:%s,timestamp:%f",
//          client->sock, inet_ntoa(client->addr.sin_addr), intersectionOverflowAlarm.timestamp);
    //存入队列
    auto server = (FusionServer *) client->super;

    if (!server->dataUnitIntersectionOverflowAlarm.pushI(intersectionOverflowAlarm,
                                                         server->FindIndexInUnOrder(
                                                                 intersectionOverflowAlarm.hardCode))) {
        LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " IntersectionOverflowAlarm,丢弃消息";
        ret = -1;
    }
    return ret;
}

static int PkgProcessFun_CmdTrafficFlowGather(ClientInfo *client, string content) {
    int ret = 0;
//    Debug("client-%d ip:%s,TrafficFlow", client->sock, inet_ntoa(client->addr.sin_addr));
    //打印下接收的内容
//    Info("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "TrafficFlowGather json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    TrafficFlow trafficFlow;
    trafficFlow.JsonUnmarshal(in);

//    Debug("trafficFlow client-%d,timestamp:%f,flowData size:%d",
//          client->sock, trafficFlow.timstamp, trafficFlow.flowData.size());

    //存入队列
    auto server = (FusionServer *) client->super;
    if (!server->dataUnitTrafficFlowGather.pushI(trafficFlow,
                                                 server->FindIndexInUnOrder(trafficFlow.hardCode))) {
        LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " TrafficFlowGather,丢弃消息";
        ret = -1;
    }
    return ret;
}

static int PkgProcessFun_CmdInWatchData_1_3_4(ClientInfo *client, string content) {
    int ret = 0;
//    Debug("client-%d ip:%s,TrafficFlow", client->sock, inet_ntoa(client->addr.sin_addr));
    //打印下接收的内容
//    Info("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "InWatchData_1_3_4 json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    InWatchData_1_3_4 inWatchData134;
    inWatchData134.JsonUnmarshal(in);

//    Debug("trafficFlow client-%d,timestamp:%f,flowData size:%d",
//          client->sock, trafficFlow.timstamp, trafficFlow.flowData.size());

    //存入队列
    auto server = (FusionServer *) client->super;
    if (!server->dataUnitInWatchData_1_3_4.pushI(inWatchData134,
                                                 server->FindIndexInUnOrder(inWatchData134.hardCode))) {
        LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " InWatchData_1_3_4,丢弃消息";
        ret = -1;
    }
    return ret;
}

static int PkgProcessFun_CmdInWatchData_2(ClientInfo *client, string content) {
    int ret = 0;
//    Debug("client-%d ip:%s,TrafficFlow", client->sock, inet_ntoa(client->addr.sin_addr));
    //打印下接收的内容
//    Info("%s\n", content.c_str());
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        LOG(ERROR) << "InWatchData_2 json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    InWatchData_2 inWatchData2;
    inWatchData2.JsonUnmarshal(in);

//    Debug("trafficFlow client-%d,timestamp:%f,flowData size:%d",
//          client->sock, trafficFlow.timstamp, trafficFlow.flowData.size());

    //存入队列
    auto server = (FusionServer *) client->super;
    if (!server->dataUnitInWatchData_2.pushI(inWatchData2,
                                             server->FindIndexInUnOrder(inWatchData2.hardCode))) {
        LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << " InWatchData_2,丢弃消息";
        ret = -1;
    }
    return ret;
}


typedef int (*PkgProcessFun)(ClientInfo *client, string content);

static map<CmdType, PkgProcessFun> PkgProcessMap = {
        make_pair(CmdResponse, PkgProcessFun_CmdResponse),
        make_pair(CmdLogin, PkgProcessFun_CmdLogin),
        make_pair(CmdHeartBeat, PkgProcessFun_CmdHeartBeat),
        make_pair(CmdFusionData, PkgProcessFun_CmdFusionData),
        make_pair(CmdCrossTrafficJamAlarm, PkgProcessFun_CmdCrossTrafficJamAlarm),
        make_pair(CmdIntersectionOverflowAlarm, PkgProcessFun_CmdIntersectionOverflowAlarm),
        make_pair(CmdTrafficFlowGather, PkgProcessFun_CmdTrafficFlowGather),
        make_pair(CmdInWatchData_1_3_4, PkgProcessFun_CmdInWatchData_1_3_4),
        make_pair(CmdInWatchData_2, PkgProcessFun_CmdInWatchData_2),
};


int ClientInfo::ThreadGetPkgContent(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return -1;
    }

    auto client = (ClientInfo *) pClientInfo;

    LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << __FUNCTION__ << " run";
    while (client->isConnect) {
        Pkg pkg;
        if (!client->queuePkg.pop(pkg)) {
            continue;
        }
//        Info("pkg cmd:%d", pkg.head.cmd);

        //按照cmd分别处理
        auto iter = PkgProcessMap.find(CmdType(pkg.head.cmd));
        if (iter != PkgProcessMap.end()) {
            iter->second(client, pkg.body);
        } else {
            //最后没有对应的方法名
            LOG(ERROR) << "client:" << inet_ntoa(client->addr.sin_addr)
                       << "最后没有对应的方法名:" << pkg.head.cmd << ",内容:" << pkg.body;
        }
    }
    LOG(INFO) << "client ip:" << inet_ntoa(client->addr.sin_addr) << __FUNCTION__ << " exit";
    return 0;
}
