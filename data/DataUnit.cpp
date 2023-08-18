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

DataUnitTrafficFlowGather::DataUnitTrafficFlowGather(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitTrafficFlowGather::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitTrafficFlowGather";
    LOG(INFO) << name << " fs_i:" << this->fs_i;
    timerBusinessName = "DataUnitTrafficFlowGather";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(10, std::bind(task, this));
}

void DataUnitTrafficFlowGather::task(void *local) {
    auto dataUnit = (DataUnitTrafficFlowGather *) local;

    dataUnit->runTask(std::bind(DataUnitTrafficFlowGather::FindOneFrame, dataUnit, dataUnit->cache / 2));
}

void DataUnitTrafficFlowGather::FindOneFrame(DataUnitTrafficFlowGather *dataUnit, int offset) {
    DataUnit::FindOneFrame(dataUnit, offset);
    //调用后续的处理
    dataUnit->TaskProcessOneFrame();
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

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitCrossTrafficJamAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitCrossTrafficJamAlarm";
    timerBusinessName = "DataUnitCrossTrafficJamAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitCrossTrafficJamAlarm::task(void *local) {
    auto dataUnit = (DataUnitCrossTrafficJamAlarm *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}

DataUnitIntersectionOverflowAlarm::DataUnitIntersectionOverflowAlarm(int c, int fs, int i_num, int cache,
                                                                     void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitIntersectionOverflowAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitIntersectionOverflowAlarm";
    timerBusinessName = "DataUnitIntersectionOverflowAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitIntersectionOverflowAlarm::task(void *local) {
    auto dataUnit = (DataUnitIntersectionOverflowAlarm *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}


DataUnitInWatchData_1_3_4::DataUnitInWatchData_1_3_4(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}


void DataUnitInWatchData_1_3_4::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitInWatchData_1_3_4";
    timerBusinessName = "DataUnitInWatchData_1_3_4";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitInWatchData_1_3_4::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_1_3_4 *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}

DataUnitInWatchData_2::DataUnitInWatchData_2(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitInWatchData_2::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitInWatchData_2";
    timerBusinessName = "DataUnitInWatchData_2";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitInWatchData_2::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_2 *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}

DataUnitStopLinePassData::DataUnitStopLinePassData(int c, int fs, int i_num, int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitStopLinePassData::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitStopLinePassData";
    timerBusinessName = "DataUnitStopLinePassData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitStopLinePassData::task(void *local) {
    auto dataUnit = (DataUnitStopLinePassData *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}

DataUnitHumanData::DataUnitHumanData(int c, int fs, int i_num, int cache, void *owner) : DataUnit(
        c, fs, i_num, cache, owner) {

}

void DataUnitHumanData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    name = "DataUnitHumanData";
    timerBusinessName = "DataUnitHumanData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitHumanData::task(void *local) {
    auto dataUnit = (DataUnitHumanData *) local;
    dataUnit->runTask(std::bind(DataUnitHumanData::specialBusiness, dataUnit));
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

DataUnitAbnormalStopData::DataUnitAbnormalStopData(int c, int fs, int i_num, int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitAbnormalStopData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    name = "DataUnitAbnormalStopData";
    timerBusinessName = "DataUnitAbnormalStopData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitAbnormalStopData::task(void *local) {
    auto dataUnit = (DataUnitAbnormalStopData *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}

DataUnitLongDistanceOnSolidLineAlarm::DataUnitLongDistanceOnSolidLineAlarm(int c, int fs, int i_num,
                                                                           int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitLongDistanceOnSolidLineAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitLongDistanceOnSolidLineAlarm";
    timerBusinessName = "DataUnitLongDistanceOnSolidLineAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitLongDistanceOnSolidLineAlarm::task(void *local) {
    auto dataUnit = (DataUnitLongDistanceOnSolidLineAlarm *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}


DataUnitHumanLitPoleData::DataUnitHumanLitPoleData(int c, int fs, int i_num,
                                                   int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitHumanLitPoleData::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    name = "DataUnitHumanLitPoleData";
    timerBusinessName = "DataUnitHumanLitPoleData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitHumanLitPoleData::task(void *local) {
    auto dataUnit = (DataUnitHumanLitPoleData *) local;
    dataUnit->runTask(std::bind(DataUnit::TransparentTransmission, dataUnit));
}
