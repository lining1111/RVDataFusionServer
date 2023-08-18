//
// Created by lining on 11/21/22.
//

#include "DataUnit.h"
#include <fstream>
#include <cmath>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <future>
#include <glog/logging.h>
#include "Data.h"


void DataUnitTrafficFlowGather::task() {
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

void DataUnitCrossTrafficJamAlarm::task() {
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

void DataUnitIntersectionOverflowAlarm::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitInWatchData_1_3_4::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitInWatchData_2::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitStopLinePassData::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitHumanData::task() {
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

void DataUnitAbnormalStopData::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitLongDistanceOnSolidLineAlarm::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}

void DataUnitHumanLitPoleData::task() {
    this->runTask(std::bind(DataUnit::TransparentTransmission, this));
}
