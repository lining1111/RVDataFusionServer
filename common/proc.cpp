//
// Created by lining on 2023/4/23.
//

#include "proc.h"
#include "common.h"
#include "config.h"
#include "data/Data.h"
#include <glog/logging.h>
#include <fmt/core.h>
#include <fmt/ranges.h>


void CacheTimestamp::update(int index, uint64_t timestamp, int caches) {
    pthread_mutex_lock(&mtx);
    //如果已经更新了就不再进行下面的操作
    if (!isSetInterval) {
        //判断是否设置了index，index默认为-1
        if (dataIndex == -1) {
            dataIndex = index;
        } else {
            //判断是否是对应的index
            if (dataIndex == index) {
                //是对应的index的话，将时间戳存进队列
                dataTimestamps.push_back(timestamp);
//                printf("存入index %d timestamp %d\n", index, timestamp);
//                printf("已存的容量%d\n", dataTimestamps.size());
            }
        }
        //如果存满缓存帧后，计算帧率，并设置标志位
        if (dataTimestamps.size() == caches) {
            std::vector<uint64_t> intervals;
            for (int i = 1; i < dataTimestamps.size(); i++) {
                auto cha = dataTimestamps.at(i) - dataTimestamps.at(i - 1);
                if (cha > 0) {
                    intervals.push_back(cha);
                }
            }
            //计算差值的平均数
            uint64_t sum = 0;
            for (auto iter: intervals) {
                sum += iter;
            }
            this->interval = sum / intervals.size();
//            printf("帧率的差和为%d\n", sum);
//            printf("计算的帧率为%d\n", this->interval);
            this->isSetInterval = true;
        }
    }
    pthread_mutex_unlock(&mtx);
}

int PkgProcessFun_CmdResponse(string ip, uint16_t port, string content) {
    VLOG(2) << "应答指令";
    return 0;
}

int PkgProcessFun_CmdControl(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "Control json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    Control control;
    control.JsonUnmarshal(in);
    LOG(INFO) << "control:" << content << "," << control.isSendVideoInfo;
    //处理控制命令
    for (auto &iter: localConfig.isSendPIC) {
        if ((iter.ip == ip) && (iter.port == port)) {
            iter.isEnable = control.isSendVideoInfo;
            break;
        }
    }

    return ret;

}

int PkgProcessFun_CmdHeartBeat(string ip, uint16_t port, string content) {
    VLOG(2) << "心跳指令";
    return 0;
}

bool issave = false;


CacheTimestamp CT_fusionData;

int PkgProcessFun_CmdFusionData(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
//    if (!issave){
//        std::ofstream	OsWrite("recv.json",std::ofstream::app);
//        OsWrite<<content;
//        OsWrite<<std::endl;
//        OsWrite.close();
//        issave=true;
//    }


    if (!reader.parse(content, in, false)) {
        VLOG(2) << "watchData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    WatchData watchData;
    watchData.JsonUnmarshal(in);

    auto *data = Data::instance();
    auto dataUnit = data->dataUnitFusionData;
    int index = -1;
    for (int i = 0; i < ARRAY_SIZE(data->roadDirection); i++) {
        if (data->roadDirection[i] == watchData.direction) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return -1;
    }

    //存到帧率缓存
    auto ct = &CT_fusionData;
    ct->update(watchData.direction, watchData.timstamp, dataUnit->cache);
    pthread_mutex_lock(&ct->mtx);
    if (ct->isSetInterval) {
        if (!ct->isStartTask) {
            ct->isStartTask = true;
            dataUnit->init(15, ct->interval, localConfig.roadNum, 15, data);
        }
    }
    pthread_mutex_unlock(&ct->mtx);

    //存入队列
    if (!dataUnit->pushI(watchData, index)) {
        VLOG(2) << "client ip:" << ip << " WatchData,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " WatchData,存入消息,"
                << "hardCode:" << watchData.hardCode << " crossID:" << watchData.matrixNo
                << "timestamp:" << (uint64_t) watchData.timstamp << " dataUnit i_vector index:"
                << index;
    }

    return ret;
}


int PkgProcessFun_CmdCrossTrafficJamAlarm(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "CrossTrafficJamAlarm json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    CrossTrafficJamAlarm crossTrafficJamAlarm;
    crossTrafficJamAlarm.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitCrossTrafficJamAlarm;
    int index = dataUnit->FindIndexInUnOrder(crossTrafficJamAlarm.hardCode);
    if (!dataUnit->pushI(crossTrafficJamAlarm, index)) {
        VLOG(2) << "client ip:" << ip << " CrossTrafficJamAlarm,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " CrossTrafficJamAlarm,存入消息,"
                << "hardCode:" << crossTrafficJamAlarm.hardCode << " crossID:" << crossTrafficJamAlarm.crossID
                << "timestamp:" << (uint64_t) crossTrafficJamAlarm.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_CmdIntersectionOverflowAlarm(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "IntersectionOverflowAlarm json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    IntersectionOverflowAlarm intersectionOverflowAlarm;
    intersectionOverflowAlarm.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitIntersectionOverflowAlarm;
    int index = dataUnit->FindIndexInUnOrder(intersectionOverflowAlarm.hardCode);
    if (!dataUnit->pushI(intersectionOverflowAlarm, index)) {
        VLOG(2) << "client ip:" << ip << " IntersectionOverflowAlarm,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " IntersectionOverflowAlarm,存入消息,"
                << "hardCode:" << intersectionOverflowAlarm.hardCode
                << " crossID:" << intersectionOverflowAlarm.crossID
                << "timestamp:" << (uint64_t) intersectionOverflowAlarm.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

CacheTimestamp CT_trafficFlowGather;

int PkgProcessFun_CmdTrafficFlowGather(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "TrafficFlowGather json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    TrafficFlow trafficFlow;
    trafficFlow.JsonUnmarshal(in);

    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitTrafficFlowGather;
    int index = dataUnit->FindIndexInUnOrder(trafficFlow.hardCode);

    //存到帧率缓存
    auto ct = &CT_trafficFlowGather;
    ct->update(index, trafficFlow.timestamp, dataUnit->cache);
    pthread_mutex_lock(&ct->mtx);
    if (ct->isSetInterval) {
        if (!ct->isStartTask) {
            ct->isStartTask = true;
            dataUnit->init(3, ct->interval, localConfig.roadNum, 3, data);
        }
    }
    pthread_mutex_unlock(&ct->mtx);

    if (!dataUnit->pushI(trafficFlow, index)) {
        VLOG(2) << "client ip:" << ip << " TrafficFlowGather,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " TrafficFlow,存入消息,"
                << "hardCode:" << trafficFlow.hardCode << " crossID:" << trafficFlow.crossID
                << "timestamp:" << (uint64_t) trafficFlow.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_CmdInWatchData_1_3_4(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "InWatchData_1_3_4 json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    InWatchData_1_3_4 inWatchData134;
    inWatchData134.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitInWatchData_1_3_4;
    int index = dataUnit->FindIndexInUnOrder(inWatchData134.hardCode);
    if (!dataUnit->pushI(inWatchData134, index)) {
        VLOG(2) << "client ip:" << ip << " InWatchData_1_3_4,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " InWatchData_1_3_4,存入消息,"
                << "hardCode:" << inWatchData134.hardCode << " crossID:" << inWatchData134.crossID
                << "timestamp:" << (uint64_t) inWatchData134.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_CmdInWatchData_2(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "InWatchData_2 json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    InWatchData_2 inWatchData2;
    inWatchData2.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitInWatchData_2;
    int index = dataUnit->FindIndexInUnOrder(inWatchData2.hardCode);
    if (!dataUnit->pushI(inWatchData2, index)) {
        VLOG(2) << "client ip:" << ip << " InWatchData_2,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " InWatchData_2,存入消息,"
                << "hardCode:" << inWatchData2.hardCode << " crossID:" << inWatchData2.crossID
                << "timestamp:" << (uint64_t) inWatchData2.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_StopLinePassData(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "StopLinePassData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    StopLinePassData stopLinePassData;
    stopLinePassData.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitStopLinePassData;
    int index = dataUnit->FindIndexInUnOrder(stopLinePassData.hardCode);
    if (!dataUnit->pushI(stopLinePassData, index)) {
        VLOG(2) << "client ip:" << ip << " StopLinePassData,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " StopLinePassData,存入消息,"
                << "hardCode:" << stopLinePassData.hardCode << " crossID:" << stopLinePassData.crossID
                << "timestamp:" << (uint64_t) stopLinePassData.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_AbnormalStopData(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "AbnormalStopData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    AbnormalStopData abnormalStopData;
    abnormalStopData.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitAbnormalStopData;
    int index = dataUnit->FindIndexInUnOrder(abnormalStopData.hardCode);
    if (!dataUnit->pushI(abnormalStopData, index)) {
        VLOG(2) << "client ip:" << ip << " AbnormalStopData,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " AbnormalStopData,存入消息,"
                << "hardCode:" << abnormalStopData.hardCode << " crossID:" << abnormalStopData.crossID
                << "timestamp:" << (uint64_t) abnormalStopData.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_LongDistanceOnSolidLineAlarm(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "LongDistanceOnSolidLineAlarm json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    LongDistanceOnSolidLineAlarm longDistanceOnSolidLineAlarm;
    longDistanceOnSolidLineAlarm.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitLongDistanceOnSolidLineAlarm;
    int index = dataUnit->FindIndexInUnOrder(longDistanceOnSolidLineAlarm.hardCode);
    if (!dataUnit->pushI(longDistanceOnSolidLineAlarm, index)) {
        VLOG(2) << "client ip:" << ip << " LongDistanceOnSolidLineAlarm,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " LongDistanceOnSolidLineAlarm,存入消息,"
                << "hardCode:" << longDistanceOnSolidLineAlarm.hardCode << " crossID:"
                << longDistanceOnSolidLineAlarm.crossID
                << "timestamp:" << (uint64_t) longDistanceOnSolidLineAlarm.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_HumanData(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "HumanData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    HumanData humanData;
    humanData.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitHumanData;
    int index = dataUnit->FindIndexInUnOrder(humanData.hardCode);
    if (!dataUnit->pushI(humanData, index)) {
        VLOG(2) << "client ip:" << ip << " HumanData,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " HumanData,存入消息,"
                << "hardCode:" << humanData.hardCode << " crossID:" << humanData.crossID
                << "timestamp:" << (uint64_t) humanData.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}

int PkgProcessFun_HumanLitPoleData(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "HumanLitPoleData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    HumanLitPoleData humanLitPoleData;
    humanLitPoleData.JsonUnmarshal(in);
    //存入队列
//    auto server = (FusionServer *) client->super;
    auto *data = Data::instance();
    auto dataUnit = data->dataUnitHumanLitPoleData;
    int index = dataUnit->FindIndexInUnOrder(humanLitPoleData.hardCode);
    if (!dataUnit->pushI(humanLitPoleData, index)) {
        VLOG(2) << "client ip:" << ip << " HumanLitPoleData,丢弃消息";
        ret = -1;
    } else {
        VLOG(2) << "client ip:" << ip << " HumanLitPoleData,存入消息,"
                << "hardCode:" << humanLitPoleData.hardCode << " crossID:" << humanLitPoleData.crossID
                << "timestamp:" << (uint64_t) humanLitPoleData.timestamp << " dataUnit i_vector index:"
                << index;
    }
    return ret;
}


int PkgProcessFun_0xf1(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "TrafficData json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    TrafficData trafficData;
    trafficData.JsonUnmarshal(in);
    VLOG(2) << "0xf1 cmd recv:vehicleCount " << trafficData.vehicleCount;
    //根据人数和车数进行设置
    uint8_t num = 0;
    if (trafficData.vehicleCount > 0) {
        num = 1;
    }

    if (signalControl->isOpen) {
        //发送14
        using namespace ComFrame_GBT20999_2017;
        FrameAll frameSend;
        frameSend.version = 0x0100;
        frameSend.controlCenterID = 1;
        frameSend.roadTrafficSignalControllerID = 21;
        frameSend.roadID = 1;
        frameSend.sequence = 1;
        frameSend.type = ComFrame_GBT20999_2017::Type_Set;
        frameSend.dataItemNum = 1;
        ComFrame_GBT20999_2017::DataItem dataItem;
        dataItem.index = 1;
        dataItem.length = 13;
        dataItem.typeID = 192;
        dataItem.objID = 2;
        dataItem.attrID = 1;
        dataItem.elementID = 0;
        dataItem.data.push_back(0);

        dataItem.data.push_back(0xfe);
        dataItem.data.push_back(7);

        dataItem.data.push_back(0);

        dataItem.data.push_back(0);
        dataItem.data.push_back(3);

        dataItem.data.push_back(1);

        dataItem.data.push_back(1);

        dataItem.data.push_back(num);

        dataItem.data.push_back(0xff);

        frameSend.dataItems.push_back(dataItem);

        vector<uint8_t> bytesSend;
        frameSend.setToBytesWithTF(bytesSend);
        VLOG(2) << fmt::format("bytesSend:{::#x}", bytesSend);

        FrameAll frameRecv;
        if (signalControl->Communicate(frameSend, frameRecv) == 0) {
            vector<uint8_t> bytesRecv;
            frameRecv.setToBytes(bytesRecv);
            VLOG(2) << fmt::format("bytesRecv:{::#x}", bytesRecv);
        }

    } else {
        VLOG(2) << "信控机未打开";
        ret = -1;
    }

    return ret;
}

int PkgProcessFun_0xf2(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "AlarmBroken json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    AlarmBroken alarmBroken;
    alarmBroken.JsonUnmarshal(in);
    VLOG(2) << "0xf2 cmd recv";
    //根据

//    if (signalControl->isOpen) {
//        //发送15或者16
//        using namespace ComFrame_GBT20999_2017;
//        FrameAll frameSend;
//        frameSend.version = 0x0100;
//        frameSend.controlCenterID = 1;
//        frameSend.roadTrafficSignalControllerID = 21;
//        frameSend.roadID = 1;
//        frameSend.sequence = 1;
//        frameSend.type = ComFrame_GBT20999_2017::Type_Trap;
//        frameSend.dataItemNum = 1;
//        ComFrame_GBT20999_2017::DataItem dataItem;
//        dataItem.index = 1;
//        dataItem.length = 4;
//        dataItem.typeID = 15;
//        dataItem.objID = 2;
//        dataItem.attrID = 2;
//        dataItem.elementID = 0;
//        //待加入
//        frameSend.dataItems.push_back(dataItem);
//
//        vector<uint8_t> bytesSend;
//        frameSend.setToBytesWithTF(bytesSend);
//        VLOG(2) << fmt::format("bytesSend:{::#x}", bytesSend);
//
//        FrameAll frameRecv;
//        if (signalControl->Communicate(frameSend, frameRecv) == 0) {
//            vector<uint8_t> bytesRecv;
//            frameRecv.setToBytes(bytesRecv);
//            VLOG(2) << fmt::format("bytesRecv:{::#x}", bytesRecv);
//        }
//
//    } else {
//        VLOG(2) << "信控机未打开";
//        ret = -1;
//    }

    return ret;
}

int PkgProcessFun_0xf3(string ip, uint16_t port, string content) {
    int ret = 0;
    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(content, in, false)) {
        VLOG(2) << "UrgentPriority json 解析失败:" << reader.getFormattedErrorMessages();
        return -1;
    }
    UrgentPriority urgentPriority;
    urgentPriority.JsonUnmarshal(in);
    VLOG(2) << "0xf3 cmd recv";
    //根据

//    if (signalControl->isOpen) {
//        //发送8
//        using namespace ComFrame_GBT20999_2017;
//        FrameAll frameSend;
//        frameSend.version = 0x0100;
//        frameSend.controlCenterID = 1;
//        frameSend.roadTrafficSignalControllerID = 21;
//        frameSend.roadID = 1;
//        frameSend.sequence = 1;
//        frameSend.type = ComFrame_GBT20999_2017::Type_Trap;
//        frameSend.dataItemNum = 1;
//        ComFrame_GBT20999_2017::DataItem dataItem;
//        dataItem.index = 1;
//        dataItem.length = 4;
//        dataItem.typeID = 8;
//        dataItem.objID = 2;
//        dataItem.attrID = 2;
//        dataItem.elementID = 0;
//        //待加入
//        frameSend.dataItems.push_back(dataItem);
//
//        vector<uint8_t> bytesSend;
//        frameSend.setToBytesWithTF(bytesSend);
//        VLOG(2) << fmt::format("bytesSend:{::#x}", bytesSend);
//
//        FrameAll frameRecv;
//        if (signalControl->Communicate(frameSend, frameRecv) == 0) {
//            vector<uint8_t> bytesRecv;
//            frameRecv.setToBytes(bytesRecv);
//            VLOG(2) << fmt::format("bytesRecv:{::#x}", bytesRecv);
//        }
//
//    } else {
//        VLOG(2) << "信控机未打开";
//        ret = -1;
//    }

    return ret;
}

map<CmdType, PkgProcessFun> PkgProcessMap = {
        make_pair(CmdResponse, PkgProcessFun_CmdResponse),
        make_pair(CmdControl, PkgProcessFun_CmdControl),
        make_pair(CmdHeartBeat, PkgProcessFun_CmdHeartBeat),
        make_pair(CmdFusionData, PkgProcessFun_CmdFusionData),
        make_pair(CmdCrossTrafficJamAlarm, PkgProcessFun_CmdCrossTrafficJamAlarm),
        make_pair(CmdIntersectionOverflowAlarm, PkgProcessFun_CmdIntersectionOverflowAlarm),
        make_pair(CmdTrafficFlowGather, PkgProcessFun_CmdTrafficFlowGather),
        make_pair(CmdInWatchData_1_3_4, PkgProcessFun_CmdInWatchData_1_3_4),
        make_pair(CmdInWatchData_2, PkgProcessFun_CmdInWatchData_2),
        make_pair(CmdStopLinePassData, PkgProcessFun_StopLinePassData),
        make_pair(CmdAbnormalStopData, PkgProcessFun_AbnormalStopData),
        make_pair(CmdLongDistanceOnSolidLineAlarm, PkgProcessFun_LongDistanceOnSolidLineAlarm),
        make_pair(CmdHumanData, PkgProcessFun_HumanData),
        make_pair(CmdHumanLitPoleData, PkgProcessFun_HumanLitPoleData),

        //信控机测试
        make_pair((CmdType) 0xf1, PkgProcessFun_0xf1),
        make_pair((CmdType) 0xf2, PkgProcessFun_0xf2),
        make_pair((CmdType) 0xf3, PkgProcessFun_0xf3),
};



