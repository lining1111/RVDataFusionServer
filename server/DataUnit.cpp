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
#include "log/Log.h"

using namespace z_log;

DataUnitMultiViewCarTracks::DataUnitMultiViewCarTracks() {

}

DataUnitMultiViewCarTracks::~DataUnitMultiViewCarTracks() {

}

DataUnitMultiViewCarTracks::DataUnitMultiViewCarTracks(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitMultiViewCarTracks::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        if (sizeI(i) > maxPkgs) {
            maxPkgs = sizeI(i);
            maxPkgsIndex = i;
        }
    }
    //未达到缓存数，退出
    if (maxPkgs < cache) {
        return -1;
    }
    //2确定标定的时间戳
    IType refer;
    if (isFront) {
        frontI(refer, maxPkgsIndex);

    } else {
        backI(refer, maxPkgsIndex);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        Debug("当前时间戳：%lu小于缓存阈值:%lu", (uint64_t) refer.timstamp, (uint64_t) timestampThreshold);
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = (uint64_t) refer.timstamp;
    }
    Debug("取同一帧时,标定时间戳为:lu", curTimestamp);
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, this, i, leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }


    //打印下每一路取到的时间戳
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            Debug("第%d路取到的时间戳为%lu", i, (uint64_t) iter.timstamp);
        }
    }

    //调用后续的处理
    if (task != nullptr) {
        return task(this);
    }

    return 0;
}

int DataUnitMultiViewCarTracks::ThreadGetDataInRange(DataUnitMultiViewCarTracks *dataUnit,
                                                     int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            Debug("第%d路数据为空", index);
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //记录路口编号
                            dataUnit->crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                            Debug("第%d路时间戳较旧但只有1帧，保留:%lu", index, (uint64_t) refer.timstamp);
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        Debug("第%d路时间戳较旧，舍弃:%lu", index, (uint64_t) refer.timstamp);
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //记录路口编号
                        dataUnit->crossID = cur.crossCode;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                        Debug("第%d路时间戳在范围内，保留:%lu", index, (uint64_t) refer.timstamp);
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    Debug("第%d路时间戳较新，保留:%lu", index, (uint64_t) refer.timstamp);
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitMultiViewCarTracks::TaskProcessOneFrame(DataUnitMultiViewCarTracks *dataUnit) {
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = dataUnit->crossID;

    for (auto iter:dataUnit->oneFrame) {
        if (!iter.lstObj.empty()) {
            for (auto iter1:iter.lstObj) {
                item.lstObj.push_back(iter1);
            }
        }
    }
    if (!dataUnit->pushO(item)) {
        Debug("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, (uint64_t) item.timestamp);
        return -1;
    } else {
        Debug("%s数据存入,timestamp:%lu", __FUNCTION__, (uint64_t) item.timestamp);
        return 0;
    }
}


DataUnitTrafficFlows::DataUnitTrafficFlows() {

}

DataUnitTrafficFlows::~DataUnitTrafficFlows() {

}

DataUnitTrafficFlows::DataUnitTrafficFlows(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitTrafficFlows::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        if (sizeI(i) > maxPkgs) {
            maxPkgs = sizeI(i);
            maxPkgsIndex = i;
        }
    }
    //未达到缓存数，退出
    if (maxPkgs < cache) {
        return -1;
    }
    //2确定标定的时间戳
    IType refer;
    if (isFront) {
        frontI(refer, maxPkgsIndex);
    } else {
        backI(refer, maxPkgsIndex);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        Debug("当前时间戳：%lu小于缓存阈值:%lu", (uint64_t) refer.timstamp, (uint64_t) timestampThreshold);
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = (uint64_t) refer.timstamp;
    }
    Debug("取同一帧时,标定时间戳为:lu", curTimestamp);
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, this, i, leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }

    //打印下每一路取到的时间戳
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            Debug("第%d路取到的时间戳为%lu", i, (uint64_t) iter.timstamp);
        }
    }
    //调用后续的处理
    if (task != nullptr) {
        return task(this);
    }

    return 0;
}

int DataUnitTrafficFlows::ThreadGetDataInRange(DataUnitTrafficFlows *dataUnit,
                                               int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            Debug("第%d路数据为空", index);
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //记录路口编号
                            dataUnit->crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                            Debug("第%d路时间戳较旧但只有1帧，保留:%lu", index, (uint64_t) refer.timstamp);
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        Debug("第%d路时间戳较旧，舍弃:%lu", index, (uint64_t) refer.timstamp);
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //记录路口编号
                        dataUnit->crossID = cur.crossCode;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                        Debug("第%d路时间戳在范围内，保留:%lu", index, (uint64_t) refer.timstamp);
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    Debug("第%d路时间戳较新，保留:%lu", index, (uint64_t) refer.timstamp);
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitTrafficFlows::TaskProcessOneFrame(DataUnitTrafficFlows *dataUnit) {
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = dataUnit->crossID;

    for (auto iter:dataUnit->oneFrame) {
        if (!iter.flowData.empty()) {
            for (auto iter1:iter.flowData) {
                item.trafficFlow.push_back(iter1);
            }
        }
    }
    if (!dataUnit->pushO(item)) {
        Debug("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, (uint64_t) item.timestamp);
        return -1;
    } else {
        Debug("%s数据存入,timestamp:%lu", __FUNCTION__, (uint64_t) item.timestamp);
        return 0;
    }
}


DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm() {

}

DataUnitCrossTrafficJamAlarm::~DataUnitCrossTrafficJamAlarm() {

}

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitCrossTrafficJamAlarm::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        if (sizeI(i) > maxPkgs) {
            maxPkgs = sizeI(i);
            maxPkgsIndex = i;
        }
    }
    //未达到缓存数，退出
    if (maxPkgs < cache) {
        return -1;
    }
    //2确定标定的时间戳
    IType refer;
    if (isFront) {
        frontI(refer, maxPkgsIndex);
    } else {
        backI(refer, maxPkgsIndex);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        Debug("当前时间戳：%lu小于缓存阈值:%lu", (uint64_t) refer.timstamp, (uint64_t) timestampThreshold);
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = (uint64_t) refer.timstamp;
    }
    Debug("取同一帧时,标定时间戳为:lu", curTimestamp);
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, this, i, leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            Debug("第%d路取到的时间戳为%lu", i, (uint64_t) iter.timstamp);
        }
    }

    //调用后续的处理
    if (task != nullptr) {
        return task(this);
    }

    return 0;
}

int DataUnitCrossTrafficJamAlarm::ThreadGetDataInRange(DataUnitCrossTrafficJamAlarm *dataUnit,
                                                       int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            Debug("第%d路数据为空", index);
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //记录路口编号
                            dataUnit->crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                            Debug("第%d路时间戳较旧但只有1帧，保留:%lu", index, (uint64_t) refer.timstamp);
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        Debug("第%d路时间戳较旧，舍弃:%lu", index, (uint64_t) refer.timstamp);
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //记录路口编号
                        dataUnit->crossID = cur.crossCode;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                        Debug("第%d路时间戳在范围内，保留:%lu", index, (uint64_t) refer.timstamp);
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    Debug("第%d路时间戳较新，保留:%lu", index, (uint64_t) refer.timstamp);
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit) {
    OType item;
    item.oprNum = random_uuid();
    item.timstamp = dataUnit->curTimestamp;
    item.crossCode = dataUnit->crossID;
    item.alarmType = 0;
    item.alarmStatus = 0;
    auto tp = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    item.alarmTime = ss.str();
    //判断是否有拥堵
    for (auto iter:dataUnit->oneFrame) {
        if (iter.alarmType == 1) {
            item.alarmType = iter.alarmType;
            item.alarmStatus = iter.alarmStatus;
            item.alarmTime = iter.alarmTime;
        }
    }

    if (!dataUnit->pushO(item)) {
        Debug("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, (uint64_t) item.timstamp);
        return -1;
    } else {
        Debug("%s数据存入,timestamp:%lu", __FUNCTION__, (uint64_t) item.timstamp);
        return 0;
    }
}


DataUnitLineupInfoGather::DataUnitLineupInfoGather() {

}

DataUnitLineupInfoGather::~DataUnitLineupInfoGather() {

}

DataUnitLineupInfoGather::DataUnitLineupInfoGather(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitLineupInfoGather::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        if (sizeI(i) > maxPkgs) {
            maxPkgs = sizeI(i);
            maxPkgsIndex = i;
        }
    }
    //未达到缓存数，退出
    if (maxPkgs < cache) {
        return -1;
    }
    //2确定标定的时间戳
    IType refer;
    if (isFront) {
        frontI(refer, maxPkgsIndex);
    } else {
        backI(refer, maxPkgsIndex);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        Debug("当前时间戳：%lu小于缓存阈值:%lu", (uint64_t) refer.timstamp, (uint64_t) timestampThreshold);
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = (uint64_t) refer.timstamp;
    }
    Debug("取同一帧时,标定时间戳为:lu", curTimestamp);
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, this, i, leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            Debug("第%d路取到的时间戳为%lu", i, (uint64_t) iter.timstamp);
        }
    }

    //调用后续的处理
    if (task != nullptr) {
        return task(this);
    }

    return 0;
}

int DataUnitLineupInfoGather::ThreadGetDataInRange(DataUnitLineupInfoGather *dataUnit,
                                                   int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            Debug("第%d路数据为空", index);
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //记录路口编号
                            dataUnit->crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                            Debug("第%d路时间戳较旧但只有1帧，保留:%lu", index, (uint64_t) refer.timstamp);
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        Debug("第%d路时间戳较旧，舍弃:%lu", index, (uint64_t) refer.timstamp);
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //记录路口编号
                        dataUnit->crossID = cur.crossCode;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                        Debug("第%d路时间戳在范围内，保留:%lu", index, (uint64_t) refer.timstamp);
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    Debug("第%d路时间戳较新，保留:%lu", index, (uint64_t) refer.timstamp);
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitLineupInfoGather::TaskProcessOneFrame(DataUnitLineupInfoGather *dataUnit) {
    OType item;
    item.oprNum = random_uuid();
    item.timstamp = dataUnit->curTimestamp;
    item.crossCode = dataUnit->crossID;

    for (auto iter:dataUnit->oneFrame) {
        if (!iter.trafficFlowList.empty()) {
            for (auto iter1:iter.trafficFlowList) {
                item.trafficFlowList.push_back(iter1);
            }
        }
    }
    if (!dataUnit->pushO(item)) {
        Debug("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, (uint64_t) item.timstamp);
        return -1;
    } else {
        Debug("%s数据存入,timestamp:%lu", __FUNCTION__, (uint64_t) item.timstamp);
        return 0;
    }
}


DataUnitFusionData::DataUnitFusionData() {

}

DataUnitFusionData::~DataUnitFusionData() {

}

DataUnitFusionData::DataUnitFusionData(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitFusionData::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, MergeType mergeType,
                                     bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        if (sizeI(i) > maxPkgs) {
            maxPkgs = sizeI(i);
            maxPkgsIndex = i;
        }
    }
    //未达到缓存数，退出
    if (maxPkgs < cache) {
        return -1;
    }
    //2确定标定的时间戳
    IType refer;
    if (isFront) {
        frontI(refer, maxPkgsIndex);
    } else {
        backI(refer, maxPkgsIndex);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        Debug("当前时间戳：%lu小于缓存阈值:%lu", (uint64_t) refer.timstamp, (uint64_t) timestampThreshold);
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = (uint64_t) refer.timstamp;
    }
    Debug("取同一帧时,标定时间戳为:lu", curTimestamp);
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, this, i, leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }

    //打印下每一路取到的时间戳
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            Debug("第%d路取到的时间戳为%lu", i, (uint64_t) iter.timstamp);
        }
    }

    //调用后续的处理
    if (task != nullptr) {
        return task(this, mergeType);
    }

    return 0;
}

int DataUnitFusionData::ThreadGetDataInRange(DataUnitFusionData *dataUnit,
                                             int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            Debug("第%d路数据为空", index);
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //记录路口编号
                            dataUnit->crossID = cur.matrixNo;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                            Debug("第%d路时间戳较旧但只有1帧，保留:%lu", index, (uint64_t) refer.timstamp);
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        Debug("第%d路时间戳较旧，舍弃:%lu", index, (uint64_t) refer.timstamp);
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //记录路口编号
                        dataUnit->crossID = cur.matrixNo;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame.insert(dataUnit->oneFrame.begin() + index, cur);
                        Debug("第%d路时间戳在范围内，保留:%lu", index, (uint64_t) refer.timstamp);
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    Debug("第%d路时间戳较新，保留:%lu", index, (uint64_t) refer.timstamp);
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitFusionData::TaskProcessOneFrame(DataUnitFusionData *dataUnit, DataUnitFusionData::MergeType mergeType) {
    //1，将同一帧内待输入量存入集合
    DataUnitFusionData::RoadDataInSet roadDataInSet;
    roadDataInSet.roadDataList.reserve(dataUnit->oneFrame.size());

    roadDataInSet.timestamp = dataUnit->curTimestamp;
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        DataUnitFusionData::RoadData item;
        item.hardCode = iter.hardCode;
        item.imageData = iter.imageData;
        for (int j = 0; j < iter.lstObjTarget.size(); j++) {
            auto iter1 = iter.lstObjTarget.at(j);
            OBJECT_INFO_T objectInfoT;
            //转换数据类型
            ObjTarget2OBJECT_INFO_T(iter1, objectInfoT);
            item.listObjs.push_back(objectInfoT);
        }
        //存入对应路的集合
        roadDataInSet.roadDataList.insert(roadDataInSet.roadDataList.begin() + i, item);
    }
    int ret = 0;
    switch (mergeType) {
        case DataUnitFusionData::NotMerge: {
            ret = dataUnit->TaskNotMerge(roadDataInSet);
        }
            break;
        case DataUnitFusionData::Merge: {
            ret = dataUnit->TaskMerge(roadDataInSet);
        }
            break;
    }
    return ret;
}

int DataUnitFusionData::TaskNotMerge(DataUnitFusionData::RoadDataInSet roadDataInSet) {
    MergeData mergeData;
    mergeData.timestamp = roadDataInSet.timestamp;
    mergeData.obj.clear();

    mergeData.objInput.roadDataList.reserve(roadDataInSet.roadDataList.size());

    const int INF = 0x7FFFFFFF;
    //输出量
    for (int i = 0; i < roadDataInSet.roadDataList.size(); i++) {
        auto iter = roadDataInSet.roadDataList.at(i);
        for (auto iter1:iter.listObjs) {
            OBJECT_INFO_NEW item;
            switch (i) {
                case 0: {
                    //North
                    item.objID1 = iter1.objID;
                    item.cameraID1 = iter1.cameraID;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 10000;
                }
                    break;
                case 1: {
                    //East
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = iter1.objID;
                    item.cameraID2 = iter1.cameraID;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 20000;
                }
                    break;
                case 2: {
                    //South
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = iter1.objID;
                    item.cameraID3 = iter1.cameraID;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 30000;
                }
                    break;
                case 3: {
                    //West
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = iter1.objID;
                    item.cameraID4 = iter1.cameraID;

                    item.showID = iter1.objID + 40000;
                }
                    break;
            }
            item.objType = iter1.objType;
            memcpy(item.plate_number, iter1.plate_number, sizeof(iter1.plate_number));
            memcpy(item.plate_color, iter1.plate_color, sizeof(iter1.plate_color));
            item.left = iter1.left;
            item.top = iter1.top;
            item.right = iter1.right;
            item.bottom = iter1.bottom;
            item.locationX = iter1.locationX;
            item.locationY = iter1.locationY;
            memcpy(item.distance, iter1.distance, sizeof(iter1.distance));
            item.directionAngle = iter1.directionAngle;
            item.speed = sqrt(iter1.speedX * iter1.speedX + iter1.speedY * iter1.speedY);
            item.latitude = iter1.latitude;
            item.longitude = iter1.longitude;
            item.flag_new = 0;

            mergeData.obj.push_back(item);
        }
    }

    return GetFusionData(mergeData);
}

int DataUnitFusionData::TaskMerge(RoadDataInSet roadDataInSet) {
    Info("融合:数据选取的时间戳:%lu", roadDataInSet.timestamp);
    //如果只有一路数据，不走融合
    int haveOneRoadDataCount = 0;
    for (auto iter:roadDataInSet.roadDataList) {
        if (!iter.listObjs.empty()) {
            haveOneRoadDataCount++;
        }
    }

    //存输入数据到文件
    if (0) {
        string fileName = to_string(roadDataInSet.timestamp) + "_in.txt";
        ofstream inDataFile;
        string inDataFileName = "mergeData/" + fileName;
        inDataFile.open(inDataFileName);
        string content;
        for (int i = 0; i < roadDataInSet.roadDataList.size(); i++) {
            auto iter = roadDataInSet.roadDataList.at(i);

            string split = ",";
            for (auto iter1:iter.listObjs) {
                content.append(to_string(i) + split +
                               to_string(iter1.locationX) + split +
                               to_string(iter1.locationY) + split +
                               to_string(iter1.speedX) + split +
                               to_string(iter1.speedY) + split +
                               to_string(iter1.objID) + split +
                               to_string(iter1.objType));
                content.append("\n");
            }
            //存入图片
            string inDataPicFileName =
                    "mergeData/" + to_string(roadDataInSet.timestamp) + "_in_" + to_string(i) + ".jpeg";
            ofstream inDataPicFile;
            inDataPicFile.open(inDataPicFileName);
            if (inDataPicFile.is_open()) {
                inDataPicFile.write(iter.imageData.data(), iter.imageData.size());
                Debug("融合数据图片写入");
                inDataPicFile.flush();
                inDataPicFile.close();
            }

        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) content.c_str(), content.size());
            Debug("融合数据写入");
            inDataFile.flush();
            inDataFile.close();
        }
    }


    MergeData mergeData;
    mergeData.timestamp = roadDataInSet.timestamp;
    mergeData.obj.clear();

    mergeData.objInput.roadDataList.reserve(roadDataInSet.roadDataList.size());
    switch (haveOneRoadDataCount) {
        case 0 : {
            Debug("全部路都没有数据");
        }
            break;
        case 1 : {
            Debug("只有1路有数据,不走融合");
            //结果直接存输入
            for (int i = 0; i < roadDataInSet.roadDataList.size(); i++) {
                auto iter = roadDataInSet.roadDataList.at(i);
                if (!iter.listObjs.empty()) {
                    //存目标
                    for (auto iter1:iter.listObjs) {
                        OBJECT_INFO_NEW item;
                        OBJECT_INFO_T2OBJECT_INFO_NEW(iter1, item);
                        mergeData.obj.push_back(item);
                    }
                    //存其他
                    mergeData.objInput.roadDataList.at(i).hardCode = iter.hardCode;
                    mergeData.objInput.roadDataList.at(i).imageData = iter.imageData;
                    mergeData.objInput.roadDataList.at(i).listObjs.assign(iter.listObjs.begin(), iter.listObjs.end());
                }
            }
        }
            break;
        default : {
            Debug("多于1路有数据,走融合");
            OBJECT_INFO_NEW dataOut[1000];
            memset(dataOut,
                   0, ARRAY_SIZE(dataOut) * sizeof(OBJECT_INFO_NEW));
            bool isFirstFrame = true;//如果算法缓存内有数则为假
            if (!cacheAlgorithmMerge.empty()) {
                isFirstFrame = false;
            }
            vector<OBJECT_INFO_NEW> l1_obj;//上帧输出的
            vector<OBJECT_INFO_NEW> l2_obj;//上上帧输出的
            if (isFirstFrame) {
                l1_obj.clear();
                l2_obj.clear();
            } else {
                switch (cacheAlgorithmMerge.size()) {
                    case 1: {
                        //只有1帧的话，只能取到上帧
                        l2_obj.clear();
                        l1_obj = cacheAlgorithmMerge.front();
                    }
                        break;
                    case 2: {
                        //2帧的话，可以取到上帧和上上帧，这里有上上帧出缓存的动作
                        l2_obj = cacheAlgorithmMerge.front();
                        cacheAlgorithmMerge.pop();
                        l1_obj = cacheAlgorithmMerge.front();
                    }
                        break;
                }
            }
            int num = merge_total(repateX, widthX, widthY, Xmax, Ymax, gatetx, gatety, gatex, gatey,
                                  isFirstFrame,
                                  mergeData.objInput.roadDataList.at(0).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(0).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(1).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(1).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(2).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(2).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(3).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(3).listObjs.size(),
                                  l1_obj.data(), l1_obj.size(), l2_obj.data(), l2_obj.size(),
                                  dataOut, angle_value);
            //将输出存入缓存
            vector<OBJECT_INFO_NEW> out;
            for (int i = 0; i < num; i++) {
                out.push_back(dataOut[i]);
            }
            cacheAlgorithmMerge.push(out);

            //将输出存入mergeData
            mergeData.obj.assign(out.begin(), out.end());
        }
            break;
    }

//存输出数据到文件
    if (0) {
        string fileName = to_string((uint64_t) mergeData.timestamp) + "_out.txt";
        std::ofstream inDataFile;
        string inDataFileName = "mergeData/" + fileName;
        inDataFile.open(inDataFileName);
        string contentO;
        for (int j = 0; j < mergeData.obj.size(); j++) {
            string split = ",";
            auto iter = mergeData.obj.at(j);
            contentO.append(to_string(iter.objID1) + split +
                            to_string(iter.objID2) + split +
                            to_string(iter.objID3) + split +
                            to_string(iter.objID4) + split +
                            to_string(iter.showID) + split +
                            to_string(iter.cameraID1) + split +
                            to_string(iter.cameraID2) + split +
                            to_string(iter.cameraID3) + split +
                            to_string(iter.cameraID4) + split +
                            to_string(iter.locationX) + split +
                            to_string(iter.locationY) + split +
                            to_string(iter.speed) + split +
                            to_string(iter.flag_new));
            contentO.append("\n");
        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) contentO.c_str(), contentO.size());
            inDataFile.flush();
            inDataFile.close();
        } else {
//          Error("打开文件失败:%s", inDataFileNameO.c_str());
        }
    }

    return GetFusionData(mergeData);
}

int DataUnitFusionData::GetFusionData(MergeData mergeData) {
    const int INF = 0x7FFFFFFF;
    OType item;
    item.timstamp = mergeData.timestamp;
    item.crossID = this->crossID;
    //lstObjTarget
    for (auto iter:mergeData.obj) {
        ObjMix objMix;
        objMix.objID = iter.showID;
        if (iter.objID1 != -INF) {
            RvWayObject rvWayObject;
            rvWayObject.wayNo = North;
            rvWayObject.roID = iter.objID1;
            rvWayObject.voID = iter.cameraID1;
            objMix.listRvWayObject.push_back(rvWayObject);
        }
        if (iter.objID2 != -INF) {
            RvWayObject rvWayObject;
            rvWayObject.wayNo = East;
            rvWayObject.roID = iter.objID2;
            rvWayObject.voID = iter.cameraID2;
            objMix.listRvWayObject.push_back(rvWayObject);
        }
        if (iter.objID3 != -INF) {
            RvWayObject rvWayObject;
            rvWayObject.wayNo = South;
            rvWayObject.roID = iter.objID3;
            rvWayObject.voID = iter.cameraID3;
            objMix.listRvWayObject.push_back(rvWayObject);
        }
        if (iter.objID4 != -INF) {
            RvWayObject rvWayObject;
            rvWayObject.wayNo = West;
            rvWayObject.roID = iter.objID4;
            rvWayObject.voID = iter.cameraID4;
            objMix.listRvWayObject.push_back(rvWayObject);
        }
        objMix.objType = iter.objType;
        objMix.objColor = 0;
        objMix.plates = string(iter.plate_number);
        objMix.plateColor = string(iter.plate_color);
        objMix.distance = atof(string(iter.distance).c_str());
        objMix.angle = iter.directionAngle;
        objMix.speed = iter.speed;
        objMix.locationX = iter.locationX;
        objMix.locationY = iter.locationY;
        objMix.longitude = iter.longitude;
        objMix.latitude = iter.latitude;
        objMix.flagNew = iter.flag_new;

        item.lstObjTarget.push_back(objMix);
    }

    bool isSendPicData = true;
    if (isSendPicData) {
        item.isHasImage = 1;
        //lstVideos
        for (auto iter:mergeData.objInput.roadDataList) {
            VideoData videoData;
            videoData.rvHardCode = iter.hardCode;
            videoData.imageData = iter.imageData;
            for (auto iter1:iter.listObjs) {
                VideoTargets videoTargets;
                videoTargets.cameraObjID = iter1.cameraID;
                videoTargets.left = iter1.left;
                videoTargets.top = iter1.top;
                videoTargets.right = iter1.right;
                videoTargets.bottom = iter1.bottom;
                videoData.lstVideoTargets.push_back(videoTargets);
            }
            item.lstVideos.push_back(videoData);
        }
    } else {
        item.isHasImage = 0;
        item.lstVideos.resize(0);
    }

    if (pushO(item)) {
        Debug("存入融合数据成功,timestamp:%lu", (uint64_t) item.timstamp);
    } else {
        Debug("存入融合数据失败,timestamp:%lu", (uint64_t) item.timstamp);
    }

    return 0;
}

