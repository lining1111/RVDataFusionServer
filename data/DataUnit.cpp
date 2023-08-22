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
#include "Data.h"
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
    item.oprNum = random_uuid();
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
    if (!pushO(item)) {
        VLOG(2) << name << " 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
    } else {
        VLOG(2) << name << " 数据存入 timestamp:" << (uint64_t) item.timestamp;
    }
    return 0;
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
        }
    }
}

void DataUnitCrossTrafficJamAlarm::taskI() {
    this->runTask(std::bind(DataUnitCrossTrafficJamAlarm::specialBusiness, this));
}

void DataUnitCrossTrafficJamAlarm::specialBusiness(DataUnitCrossTrafficJamAlarm *dataUnit) {
//第一次取到有报警的时间戳
    uint64_t alarmTimestamp = 0;

    for (auto &iter: dataUnit->i_queue_vector) {
        while (!iter.empty()) {
            IType cur;
            iter.getIndex(cur, 0);
            iter.eraseBegin();
            if (alarmTimestamp == 0) {
                //如果第一次赋值
                alarmTimestamp = cur.timestamp;
            } else if (abs((long long) alarmTimestamp - (long long) cur.timestamp) <= (10 * 1000)) {
                //查看当前的时间戳是否在第一次取到的10s内
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败，未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
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
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client2") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败,未获得锁 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
        }
    }

}
