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
    LOG(INFO) << "DataUnitTrafficFlowGather thresholdFrame" << this->thresholdFrame;
    timerBusinessName = "DataUnitTrafficFlowGather";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(10, std::bind(task, this));
}

void DataUnitTrafficFlowGather::task(void *local) {
    auto dataUnit = (DataUnitTrafficFlowGather *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    int maxSizeIndex = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        if (dataUnit->i_maxSizeIndex == -1) {
            //只有在第一次它为数据默认值-1的时候才执行赋值
            dataUnit->i_maxSizeIndex = maxSizeIndex;
        }
        //执行相应的流程
        FindOneFrame(dataUnit, dataUnit->cache / 2);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void DataUnitTrafficFlowGather::FindOneFrame(DataUnitTrafficFlowGather *dataUnit, int offset) {
    //1确定标定的时间戳
    IType refer;
    if (!dataUnit->getIOffset(refer, dataUnit->i_maxSizeIndex, offset)) {
        dataUnit->i_maxSizeIndexNext();
        return;
    }
    //判断上次取的时间戳和这次的一样吗
    if ((dataUnit->timestampStore + dataUnit->fs_i) > ((uint64_t) refer.timestamp)) {
        dataUnit->taskSearchCount++;
        if ((dataUnit->taskSearchCount * dataUnit->timerBusiness->getPeriodMs()) >= (dataUnit->fs_i * 2.5)) {
            //超过阈值，切下一路,重新计数
            dataUnit->i_maxSizeIndexNext();
        }
        return;
    }
    dataUnit->taskSearchCount = 0;
    dataUnit->timestampStore = refer.timestamp;

    dataUnit->curTimestamp = refer.timestamp;
//    printf("开始寻找%lu\n", dataUnit->curTimestamp);

    std::time_t t((uint64_t) dataUnit->curTimestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");

    VLOG(3) << "DataUnitTrafficFlowGather取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp << " "
            << ss.str();
    LOG(INFO) << "DataUnitTrafficFlowGather取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp << " "
              << ss.str();
//    printf("dataUnit->thresholdFrame:%d\n",dataUnit->thresholdFrame);
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    vector<IType>().swap(dataUnit->oneFrame);
    dataUnit->oneFrame.resize(dataUnit->numI);
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        ThreadGetDataInRange(dataUnit, i, dataUnit->curTimestamp);
    }

    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitTrafficFlowGather 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }
    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

int
DataUnitTrafficFlowGather::ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit, int index, uint64_t curTimestamp) {
    //找到时间戳在范围内的帧
    if (dataUnit->emptyI(index)) {
        VLOG(3) << "DataUnitFusionData第" << index << "路数据为空";
    } else {
        for (int i = 0; i < dataUnit->sizeI(index); i++) {
            IType refer;
            if (dataUnit->getIOffset(refer, index, i)) {
                if (abs((int) ((uint64_t) refer.timestamp - curTimestamp)) < dataUnit->fs_i) {
                    //在范围内
                    //记录当前路的时间戳
                    dataUnit->xRoadTimestamp[index] = (uint64_t) refer.timestamp;
                    //将当前路的所有信息缓存入对应的索引
                    dataUnit->oneFrame[index] = refer;
                    VLOG(3) << "DataUnitFusionData第" << index << "路时间戳在范围内，取出来:"
                            << (uint64_t) refer.timestamp;
                    break;
                }
            }
        }
    }

    return index;
}

int DataUnitTrafficFlowGather::TaskProcessOneFrame(DataUnitTrafficFlowGather *dataUnit) {
    auto data = (Data *) dataUnit->owner;
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = data->plateId;
    std::time_t t(item.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    item.recordDateTime = ss.str();

    for (auto iter: dataUnit->oneFrame) {
        if (!iter.flowData.empty()) {
            for (auto iter1: iter.flowData) {
                item.trafficFlow.push_back(iter1);
            }
        }
    }
    int ret = 0;
    if (!dataUnit->pushO(item)) {
        VLOG(3) << "DataUnitTrafficFlowGather 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
        ret = -1;
    } else {
        VLOG(3) << "DataUnitTrafficFlowGather 数据存入 timestamp:" << (uint64_t) item.timestamp;
        ret = 0;
    }
    return ret;
}

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitCrossTrafficJamAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitCrossTrafficJamAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitCrossTrafficJamAlarm::task(void *local) {
    auto dataUnit = (DataUnitCrossTrafficJamAlarm *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm 队列已满，未存入数据 timestamp:"
                            << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitIntersectionOverflowAlarm::DataUnitIntersectionOverflowAlarm(int c, int fs, int i_num, int cache,
                                                                     void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitIntersectionOverflowAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitIntersectionOverflowAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitIntersectionOverflowAlarm::task(void *local) {
    auto dataUnit = (DataUnitIntersectionOverflowAlarm *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm 队列已满，未存入数据 timestamp:"
                            << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}


DataUnitInWatchData_1_3_4::DataUnitInWatchData_1_3_4(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}


void DataUnitInWatchData_1_3_4::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_1_3_4";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitInWatchData_1_3_4::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_1_3_4 *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitInWatchData_1_3_4 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitInWatchData_1_3_4 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitInWatchData_2::DataUnitInWatchData_2(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitInWatchData_2::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_2";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitInWatchData_2::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_2 *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitInWatchData_2 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitInWatchData_2 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitStopLinePassData::DataUnitStopLinePassData(int c, int fs, int i_num, int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitStopLinePassData::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitStopLinePassData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitStopLinePassData::task(void *local) {
    auto dataUnit = (DataUnitStopLinePassData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitStopLinePassData 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitStopLinePassData 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitHumanData::DataUnitHumanData(int c, int fs, int i_num, int cache, void *owner) : DataUnit(
        c, fs, i_num, cache, owner) {

}

void DataUnitHumanData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitHumanData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitHumanData::task(void *local) {
    auto dataUnit = (DataUnitHumanData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
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
                    VLOG(3) << "DataUnitHumanData 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitHumanData 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitAbnormalStopData::DataUnitAbnormalStopData(int c, int fs, int i_num, int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitAbnormalStopData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitAbnormalStopData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitAbnormalStopData::task(void *local) {
    auto dataUnit = (DataUnitAbnormalStopData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitAbnormalStopData 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitAbnormalStopData 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitLongDistanceOnSolidLineAlarm::DataUnitLongDistanceOnSolidLineAlarm(int c, int fs, int i_num,
                                                                           int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitLongDistanceOnSolidLineAlarm::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitLongDistanceOnSolidLineAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitLongDistanceOnSolidLineAlarm::task(void *local) {
    auto dataUnit = (DataUnitLongDistanceOnSolidLineAlarm *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitLongDistanceOnSolidLineAlarm 队列已满，未存入数据 timestamp:"
                            << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitLongDistanceOnSolidLineAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}


DataUnitHumanLitPoleData::DataUnitHumanLitPoleData(int c, int fs, int i_num,
                                                                           int cache, void *owner)
        : DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitHumanLitPoleData::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    timerBusinessName = "DataUnitHumanLitPoleData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(TaskTimeval, std::bind(task, this));
}

void DataUnitHumanLitPoleData::task(void *local) {
    auto dataUnit = (DataUnitHumanLitPoleData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitLongDistanceOnSolidLineAlarm 队列已满，未存入数据 timestamp:"
                            << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitLongDistanceOnSolidLineAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}
