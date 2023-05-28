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

DataUnitTrafficFlowGather::DataUnitTrafficFlowGather(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitTrafficFlowGather::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    LOG(INFO) << "DataUnitTrafficFlowGather thresholdFrame" << this->thresholdFrame;
    timerBusinessName = "DataUnitTrafficFlowGather";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitTrafficFlowGather::task(void *local) {
    auto dataUnit = (DataUnitTrafficFlowGather *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), true);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void DataUnitTrafficFlowGather::FindOneFrame(DataUnitTrafficFlowGather *dataUnit, uint64_t toCacheCha, bool isFront) {
    //1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitTrafficFlowGather当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitTrafficFlowGather取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
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

int DataUnitTrafficFlowGather::ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit,
                                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timestamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timestamp;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame[index] = cur;
                            VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较旧但只有1帧，保留:"
                                    << (uint64_t) refer.timestamp;
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较旧，舍弃:"
                                << (uint64_t) refer.timestamp;
                    }
                } else if ((uint64_t(refer.timestamp) >= leftTimestamp) &&
                           (uint64_t(refer.timestamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timestamp;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame[index] = cur;
                        VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳在范围内，取出来:"
                                << (uint64_t) refer.timestamp;
                        isFind = true;
                    }
                } else if (uint64_t(refer.timestamp) > rightTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

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

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitCrossTrafficJamAlarm::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitCrossTrafficJamAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), false);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void
DataUnitCrossTrafficJamAlarm::FindOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit, uint64_t toCacheCha, bool isFront) {
    //1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitCrossTrafficJamAlarm取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitCrossTrafficJamAlarm 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }

    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

//报警信息是从缓存中取到现在的时间段为止，如果数据为空，则空，如果有数据，判断是否有报警，有则设置为报警
int DataUnitCrossTrafficJamAlarm::ThreadGetDataInRange(DataUnitCrossTrafficJamAlarm *dataUnit,
                                                       int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;

    auto curTimestamp = (rightTimestamp - leftTimestamp) / 2 + leftTimestamp;
    bool isFind = false;
    IType cur;
    bool isFrameExist = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                auto data = (Data *) dataUnit->owner;
                if (data->plateId.empty()) {
                    data->plateId = refer.crossID;
                }

                cur.oprNum = random_uuid();
                cur.timestamp = curTimestamp;
                cur.crossID = data->plateId;
                cur.hardCode = refer.hardCode;
                cur.alarmType = 0;
                cur.alarmStatus = 0;
//                VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路取到的时间戳" << (uint64_t) refer.timestamp
//                        << ",标定时间戳" << (uint64_t) curTimestamp;

                if (uint64_t(refer.timestamp) <= curTimestamp) {
                    isFrameExist = true;
                    if (refer.alarmType == 1) {
                        cur.alarmType = refer.alarmType;
                        cur.alarmStatus = refer.alarmStatus;
                        cur.alarmTime = refer.alarmTime;
                    }
                    dataUnit->popI(refer, index);
                } else if (uint64_t(refer.timestamp) > curTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    if (isFrameExist) {
        //将当前路的所有信息缓存入对应的索引
        dataUnit->oneFrame[index] = cur;
    }

    return index;
}


int DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit) {
    auto data = (Data *) dataUnit->owner;
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = data->plateId;
    item.alarmType = 0;
    item.alarmStatus = 0;
    auto tp = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    item.alarmTime = ss.str();
    //判断是否有拥堵
    for (auto iter: dataUnit->oneFrame) {
        if (iter.alarmType == 1) {
            item.alarmType = iter.alarmType;
            item.alarmStatus = iter.alarmStatus;
            item.alarmTime = iter.alarmTime;
        }
    }

    int ret = 0;
    if (!dataUnit->pushO(item)) {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
        ret = -1;
    } else {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
        ret = 0;
    }
    return ret;
}


DataUnitIntersectionOverflowAlarm::DataUnitIntersectionOverflowAlarm(int c, int threshold_ms, int i_num, int cache,
                                                                     void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitIntersectionOverflowAlarm::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitIntersectionOverflowAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), false);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void DataUnitIntersectionOverflowAlarm::FindOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit, uint64_t toCacheCha,
                                                     bool isFront) {
//1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitIntersectionOverflowAlarm当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitIntersectionOverflowAlarm取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }

    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

int DataUnitIntersectionOverflowAlarm::ThreadGetDataInRange(DataUnitIntersectionOverflowAlarm *dataUnit, int index,
                                                            uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;

    auto curTimestamp = (rightTimestamp - leftTimestamp) / 2 + leftTimestamp;
    bool isFind = false;
    IType cur;
    bool isFrameExist = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;

            if (dataUnit->frontI(refer, index)) {
                auto data = (Data *) dataUnit->owner;
                if (data->plateId.empty()) {
                    data->plateId = refer.crossID;
                }

                cur.oprNum = random_uuid();
                cur.timestamp = curTimestamp;
                cur.crossID = data->plateId;
                cur.hardCode = refer.hardCode;
                cur.alarmType = 0;
                cur.alarmStatus = 0;
                if (uint64_t(refer.timestamp) <= curTimestamp) {
                    isFrameExist = true;
                    if (refer.alarmType == 1) {
                        cur.alarmType = refer.alarmType;
                        cur.alarmStatus = refer.alarmStatus;
                        cur.alarmTime = refer.alarmTime;
                    }
                    dataUnit->popI(refer, index);
                } else if (uint64_t(refer.timestamp) > curTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    if (isFrameExist) {
        //将当前路的所有信息缓存入对应的索引
        dataUnit->oneFrame[index] = cur;
    }

    return index;
}

int DataUnitIntersectionOverflowAlarm::TaskProcessOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit) {
    OType item;
    int ret = 0;
    //将各路数据都写入
    for (auto iter: dataUnit->oneFrame) {
        item = iter;
        if (!dataUnit->pushO(item)) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
            ret = -1;
        } else {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
        }
    }


    return ret;
}

DataUnitInWatchData_1_3_4::DataUnitInWatchData_1_3_4(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}


void DataUnitInWatchData_1_3_4::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_1_3_4";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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

DataUnitInWatchData_2::DataUnitInWatchData_2(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitInWatchData_2::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_2";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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

DataUnitStopLinePassData::DataUnitStopLinePassData(int c, int threshold_ms, int i_num, int cache, void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitStopLinePassData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitStopLinePassData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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

DataUnitHumanData::DataUnitHumanData(int c, int threshold_ms, int i_num, int cache, void *owner) : DataUnit(
        c, threshold_ms, i_num, cache, owner) {

}

void DataUnitHumanData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitHumanData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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
                for (auto iter: cur.areaList) {
                    item1.humanNum += iter.humanNum;
                    item1.bicycleNum += iter.bicycleNum;
                    item1.humanType = iter.humanType;
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

DataUnitAbnormalStopData::DataUnitAbnormalStopData(int c, int threshold_ms, int i_num, int cache, void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitAbnormalStopData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitAbnormalStopData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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

DataUnitLongDistanceOnSolidLineAlarm::DataUnitLongDistanceOnSolidLineAlarm(int c, int threshold_ms, int i_num,
                                                                           int cache, void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitLongDistanceOnSolidLineAlarm::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitLongDistanceOnSolidLineAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
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
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
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