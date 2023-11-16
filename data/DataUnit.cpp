//
// Created by lining on 11/21/22.
//

#include "DataUnit.h"
#include <fstream>
#include <cmath>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <glog/logging.h>
#include <uuid/uuid.h>
#include "Data.h"
#include "DataUnitUtilty.h"
#include "localBussiness/localBusiness.h"


void DataUnitTrafficFlowGather::taskI() {
    this->runTask(std::bind(DataUnitTrafficFlowGather::FindOneFrame, this, this->cache / 2));
}

void DataUnitTrafficFlowGather::FindOneFrame(DataUnitTrafficFlowGather *dataUnit, int offset) {
    if (DataUnit::FindOneFrame(dataUnit, offset) == 0) {
        //调用后续的处理
        dataUnit->TaskProcessOneFrame();
    }
}

int DataUnitTrafficFlowGather::TaskProcessOneFrame() {
    auto data = (Data *) owner;
    OType item;
    uuid_t uuid;
    char uuid_str[37];
    memset(uuid_str, 0, 37);
    uuid_generate_time(uuid);
    uuid_unparse(uuid, uuid_str);
    item.oprNum = string(uuid_str);
    item.timestamp = curTimestamp;
    item.crossID = data->plateId;
    std::time_t t(item.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    item.recordDateTime = ss.str();

    for (auto iter: oneFrame) {
        if (!iter.flowData.empty()) {
            for (auto iter1: iter.flowData) {
                item.trafficFlow.push_back(iter1);
            }
        }
    }
    //根据车道号取排队长度最大
    getMaxQueueLenByLaneCode(item.trafficFlow);

    if (!pushO(item)) {
        VLOG(2) << name << " 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
    } else {
        VLOG(2) << name << " 数据存入 timestamp:" << (uint64_t) item.timestamp;
    }
    return 0;
}

void DataUnitTrafficFlowGather::getMaxQueueLenByLaneCode(vector<OneFlowData> &v) {
    //1.先打印下原始数据
    string src;
    for (auto iter: v) {
        src += "lancode:";
        src += iter.laneCode;
        src += ",queueLen:";
        src += iter.queueLen;
        src += "\n";
    }
    VLOG(3) << src;
    //2.遍历，根据车道号筛选出最大的排队长度
    // (原理就是：遍历数组，设当前的索引应该被引用，然后和数组内的其他元素对比，如果车道号相同且小于对方的排队长度，则不应该被引用，退出当前循环，如果该索引能够被引用，则记录到数组，到最后一起拷贝到新的结构体数组)
    //2.1遍历获取索引
    vector<int> indices;
    indices.clear();
    for (int i = 0; i < v.size(); i++) {
        auto iter = v.at(i);
        auto curLaneCode = iter.laneCode;
        auto curQueueLen = iter.queueLen;
        bool isRef = true;
        for (int j = 0; j < v.size(); j++) {
            if (i != j) {
                auto iter_t = v.at(j);
                auto laneCode_t = iter_t.laneCode;
                auto queueLen_t = iter_t.queueLen;
                if (curLaneCode == laneCode_t && curQueueLen < queueLen_t) {
                    isRef = false;
                    break;
                }
            }
        }
        if (isRef) {
            indices.push_back(i);
        }
    }
    //2.1 根据所有拷贝数组
    vector<OneFlowData> v_copy;
    v_copy.clear();
    for (auto iter: indices) {
        v_copy.push_back(v.at(iter));
    }
    //3.打印下拷贝出的数组
    string src_copy;
    for (auto iter: v_copy) {
        src_copy += "lancode:";
        src_copy += iter.laneCode;
        src_copy += ",queueLen:";
        src_copy += iter.queueLen;
        src_copy += "\n";
    }
    VLOG(3) << src_copy;
    //4.将拷贝数组设置到输出
    v = v_copy;
}

void DataUnitTrafficFlowGather::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfoFs(localConfig.summaryFs, ret, cli.second->server_ip, cli.second->server_port,
                            this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }
}

void DataUnitCrossTrafficJamAlarm::taskI() {
    this->runTask(std::bind(DataUnitCrossTrafficJamAlarm::specialBusiness, this));
}

//有状态变化才上报
int alarmStatus = -1;

void DataUnitCrossTrafficJamAlarm::specialBusiness(DataUnitCrossTrafficJamAlarm *dataUnit) {

    for (auto &iter: dataUnit->i_queue_vector) {
        while (!iter.empty()) {
            IType cur;
            iter.getIndex(cur, 0);
            iter.eraseBegin();
            if (alarmStatus == -1) {
                //如果第一次赋值
                alarmStatus = cur.alarmStatus;
                VLOG(3) << dataUnit->name << "alarmStatus" << alarmStatus << "cur.alarmStatus"
                        << " 状态第一次获取 timestamp:" << cur.timestamp;
            } else if (alarmStatus != cur.alarmStatus) {
                //状态不一致，获取状态，并上报
                VLOG(3) << dataUnit->name << "alarmStatus" << alarmStatus << "cur.alarmStatus" << cur.alarmStatus
                        << " 状态不一致上报 timestamp:" << cur.timestamp;
                alarmStatus = cur.alarmStatus;

            } else {
                //状态一致不上报
                VLOG(3) << dataUnit->name << "alarmStatus" << alarmStatus << "cur.alarmStatus" << cur.alarmStatus
                        << " 状态一致不上报 timestamp:" << cur.timestamp;
                continue;
            }


            OType item = cur;
            if (!dataUnit->pushO(item)) {
                VLOG(2) << dataUnit->name << " 队列已满，未存入数据 timestamp:"
                        << (uint64_t) item.timestamp;
            } else {
                VLOG(2) << dataUnit->name << " 数据存入 timestamp:" << (uint64_t) item.timestamp;
            }
        }
    }
}

void DataUnitCrossTrafficJamAlarm::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);

        auto path1 = "/mnt/mnt_hd/save/Json/" + this->name + "/pic/";
        savePic(item.imageData, item.timestamp, path1);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
            if (ret != 0) {
                ReSendQueue::Msg msg;
                msg.name = "client2";
                msg.pkg = pkg;
                msg.timestampFrame = item.timestamp;
                msg.type = ReSendQueue::RESEND_TYPE_NONE;
                data->reSendQueue->add(msg);
            }
        }
    }
}

void DataUnitIntersectionOverflowAlarm::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitIntersectionOverflowAlarm::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);

        auto path1 = "/mnt/mnt_hd/save/Json/" + this->name + "/pic/";
        savePic(item.imageData, item.timestamp, path1);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
            if (ret != 0) {
                ReSendQueue::Msg msg;
                msg.name = "client2";
                msg.pkg = pkg;
                msg.timestampFrame = item.timestamp;
                msg.type = ReSendQueue::RESEND_TYPE_NONE;
                data->reSendQueue->add(msg);
            }
        }
    }
}

void DataUnitInWatchData_1_3_4::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitInWatchData_1_3_4::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }

}

void DataUnitInWatchData_2::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitInWatchData_2::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfoFs(localConfig.summaryFs, ret, cli.second->server_ip, cli.second->server_port,
                            this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }

}

void DataUnitStopLinePassData::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitStopLinePassData::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }

}

void DataUnitHumanData::taskI() {
    this->runTask(std::bind(DataUnitHumanData::specialBusiness, this));
}

void DataUnitHumanData::specialBusiness(DataUnitHumanData *dataUnit) {
    for (auto &iter: dataUnit->i_queue_vector) {
        while (!iter.empty()) {
            IType cur;
            iter.getIndex(cur, 0);
            iter.eraseBegin();
            OType item;
            item.oprNum = cur.oprNum;
            item.timestamp = cur.timestamp;
            item.crossID = cur.crossID;
            item.hardCode = cur.hardCode;

            HumanDataGather_deviceListItem item1;
            item1.direction = cur.direction;
            item1.detectDirection - cur.direction;
            item1.deviceCode = "";
            item1.humanType = 0;
            item1.humanNum = 0;
            item1.bicycleNum = 0;
            for (auto iter1: cur.areaList) {
                item1.humanNum += iter1.humanNum;
                item1.bicycleNum += iter1.bicycleNum;
                item1.humanType = iter1.humanType;
            }
            item.deviceList.push_back(item1);

            if (!dataUnit->pushO(item)) {
                VLOG(2) << dataUnit->name << " 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
            } else {
                VLOG(2) << dataUnit->name << " 数据存入 timestamp:" << (uint64_t) item.timestamp;
            }
        }
    }
}

void DataUnitHumanData::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }

}

void DataUnitAbnormalStopData::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitAbnormalStopData::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);

        auto path1 = "/mnt/mnt_hd/save/Json/" + this->name + "/pic/";
        savePic(item.imageData, item.timestamp, path1);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
            if (ret != 0) {
                ReSendQueue::Msg msg;
                msg.name = "client2";
                msg.pkg = pkg;
                msg.timestampFrame = item.timestamp;
                msg.type = ReSendQueue::RESEND_TYPE_NONE;
                data->reSendQueue->add(msg);
            }
        }
    }

}

void DataUnitLongDistanceOnSolidLineAlarm::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitLongDistanceOnSolidLineAlarm::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfo(ret, cli.second->server_ip, cli.second->server_port,
                          this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }

}

void DataUnitHumanLitPoleData::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitHumanLitPoleData::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfoFs(localConfig.summaryFs, ret, cli.second->server_ip, cli.second->server_port,
                            this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }
}

void DataUnitTrafficDetectorStatus::taskI() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitTrafficDetectorStatus::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;

    //1.1是否存储json
    if (localConfig.isSaveOtherJson) {
        auto path = "/mnt/mnt_hd/save/Json/" + this->name + "/";
        saveJson(pkg.body, item.timestamp, path);
    }

    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(INFO) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            PrintSendInfoFs(localConfig.summaryFs, ret, cli.second->server_ip, cli.second->server_port,
                            this->name, timestampStart, timestampEnd, item.timestamp);
        }
    }
}