//
// Created by lining on 11/21/22.
//

#include <iostream>
#include "DataUnit.h"
#include <fstream>
#include <cmath>
#include <sstream>
#include <chrono>
#include <iomanip>
#include "log/Log.h"

using namespace z_log;

DataUnitMultiViewCarTracks::DataUnitMultiViewCarTracks() {

}

DataUnitMultiViewCarTracks::~DataUnitMultiViewCarTracks() {

}

DataUnitMultiViewCarTracks::DataUnitMultiViewCarTracks(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitMultiViewCarTracks::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task,
                                             bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        if (iter.size() > maxPkgs) {
            maxPkgs = iter.size();
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
        i_queue_vector.at(maxPkgsIndex).front(refer);
    } else {
        i_queue_vector.at(maxPkgsIndex).back(refer);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                  << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = refer.timstamp;
    }
    std::cout << "取同一帧时,标定时间戳为:" << to_string(curTimestamp) << endl;
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
        bool isFind = false;
        do {
            if (iter.empty()) {
                std::cout << "第" << to_string(i) << "路数据为空" << endl;
                isFind = true;
            } else {
                IType refer;
                if (iter.front(refer)) {
                    if (uint64_t(refer.timstamp) < leftTimestamp) {
                        //在左值外
                        if (iter.size() == 1) {
                            //取用
                            IType cur;
                            if (iter.pop(cur)) {
                                //记录当前路的时间戳
                                xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                oneFrame.insert(oneFrame.begin() + i, cur);
                                std::cout << "第" << to_string(i) << "路时间戳较旧但只有1帧，保留:"
                                          << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                          << endl;
                                isFind = true;
                            }
                        } else {
                            iter.pop(refer);
                            std::cout << "第" << to_string(i) << "路时间戳较旧，舍弃:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << endl;
                        }
                    } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                               (uint64_t(refer.timstamp) <= rightTimestamp)) {
                        //在范围内
                        IType cur;
                        if (iter.pop(cur)) {
                            //记录当前路的时间戳
                            xRoadTimestamp[i] = cur.timstamp;
                            //记录路口编号
                            crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            oneFrame.insert(oneFrame.begin() + i, cur);
                            std::cout << "第" << to_string(i) << "路时间戳在范围内，保留:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << ":" << to_string(rightTimestamp) << endl;
                            isFind = true;
                        }
                    } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                        //在右值外
                        std::cout << "第" << to_string(i) << "路时间戳较新，保留:"
                                  << to_string(uint64_t(refer.timstamp)) << ":" << to_string(rightTimestamp)
                                  << endl;
                        isFind = true;
                    }
                }
            }
        } while (!isFind);
    }
    //调用后续的处理
    if (task != nullptr) {
        task(this);
    }

    return 0;
}

void DataUnitMultiViewCarTracks::TaskProcessOneFrame(DataUnitMultiViewCarTracks *dataUnit) {
    typedef MultiViewCarTrack IType;
    typedef MultiViewCarTracks OType;


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
    if (!dataUnit->o_queue.push(item)) {
        Error("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, item.timestamp);
    } else {
        Info("%s数据存入,timestamp:%lu", __FUNCTION__, item.timestamp);
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
        auto iter = i_queue_vector.at(i);
        if (iter.size() > maxPkgs) {
            maxPkgs = iter.size();
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
        i_queue_vector.at(maxPkgsIndex).front(refer);
    } else {
        i_queue_vector.at(maxPkgsIndex).back(refer);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                  << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = refer.timstamp;
    }
    std::cout << "取同一帧时,标定时间戳为:" << to_string(curTimestamp) << endl;
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
        bool isFind = false;
        do {
            if (iter.empty()) {
                std::cout << "第" << to_string(i) << "路数据为空" << endl;
                isFind = true;
            } else {
                IType refer;
                if (iter.front(refer)) {
                    if (uint64_t(refer.timstamp) < leftTimestamp) {
                        //在左值外
                        if (iter.size() == 1) {
                            //取用
                            IType cur;
                            if (iter.pop(cur)) {
                                //记录当前路的时间戳
                                xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                oneFrame.insert(oneFrame.begin() + i, cur);
                                std::cout << "第" << to_string(i) << "路时间戳较旧但只有1帧，保留:"
                                          << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                          << endl;
                                isFind = true;
                            }
                        } else {
                            iter.pop(refer);
                            std::cout << "第" << to_string(i) << "路时间戳较旧，舍弃:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << endl;
                        }
                    } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                               (uint64_t(refer.timstamp) <= rightTimestamp)) {
                        //在范围内
                        IType cur;
                        if (iter.pop(cur)) {
                            //记录当前路的时间戳
                            xRoadTimestamp[i] = cur.timstamp;
                            //记录路口编号
                            crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            oneFrame.insert(oneFrame.begin() + i, cur);
                            std::cout << "第" << to_string(i) << "路时间戳在范围内，保留:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << ":" << to_string(rightTimestamp) << endl;
                            isFind = true;
                        }
                    } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                        //在右值外
                        std::cout << "第" << to_string(i) << "路时间戳较新，保留:"
                                  << to_string(uint64_t(refer.timstamp)) << ":" << to_string(rightTimestamp)
                                  << endl;
                        isFind = true;
                    }
                }
            }
        } while (!isFind);
    }
    //调用后续的处理
    if (task != nullptr) {
        task(this);
    }

    return 0;
}

void DataUnitTrafficFlows::TaskProcessOneFrame(DataUnitTrafficFlows *dataUnit) {
    typedef TrafficFlow IType;
    typedef TrafficFlows OType;


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
    if (!dataUnit->o_queue.push(item)) {
        Error("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, item.timestamp);
    } else {
        Info("%s数据存入,timestamp:%lu", __FUNCTION__, item.timestamp);
    }
}


DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm() {

}

DataUnitCrossTrafficJamAlarm::~DataUnitCrossTrafficJamAlarm() {

}

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitCrossTrafficJamAlarm::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task,
                                               bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        if (iter.size() > maxPkgs) {
            maxPkgs = iter.size();
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
        i_queue_vector.at(maxPkgsIndex).front(refer);
    } else {
        i_queue_vector.at(maxPkgsIndex).back(refer);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                  << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = refer.timstamp;
    }
    std::cout << "取同一帧时,标定时间戳为:" << to_string(curTimestamp) << endl;
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
        bool isFind = false;
        do {
            if (iter.empty()) {
                std::cout << "第" << to_string(i) << "路数据为空" << endl;
                isFind = true;
            } else {
                IType refer;
                if (iter.front(refer)) {
                    if (uint64_t(refer.timstamp) < leftTimestamp) {
                        //在左值外
                        if (iter.size() == 1) {
                            //取用
                            IType cur;
                            if (iter.pop(cur)) {
                                //记录当前路的时间戳
                                xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                oneFrame.insert(oneFrame.begin() + i, cur);
                                std::cout << "第" << to_string(i) << "路时间戳较旧但只有1帧，保留:"
                                          << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                          << endl;
                                isFind = true;
                            }
                        } else {
                            iter.pop(refer);
                            std::cout << "第" << to_string(i) << "路时间戳较旧，舍弃:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << endl;
                        }
                    } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                               (uint64_t(refer.timstamp) <= rightTimestamp)) {
                        //在范围内
                        IType cur;
                        if (iter.pop(cur)) {
                            //记录当前路的时间戳
                            xRoadTimestamp[i] = cur.timstamp;
                            //记录路口编号
                            crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            oneFrame.insert(oneFrame.begin() + i, cur);
                            std::cout << "第" << to_string(i) << "路时间戳在范围内，保留:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << ":" << to_string(rightTimestamp) << endl;
                            isFind = true;
                        }
                    } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                        //在右值外
                        std::cout << "第" << to_string(i) << "路时间戳较新，保留:"
                                  << to_string(uint64_t(refer.timstamp)) << ":" << to_string(rightTimestamp)
                                  << endl;
                        isFind = true;
                    }
                }
            }
        } while (!isFind);
    }
    //调用后续的处理
    if (task != nullptr) {
        task(this);
    }

    return 0;
}

void DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit) {
    typedef CrossTrafficJamAlarm IType;
    typedef CrossTrafficJamAlarm OType;


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

    if (!dataUnit->o_queue.push(item)) {
        Error("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, item.timstamp);
    } else {
        Info("%s数据存入,timestamp:%lu", __FUNCTION__, item.timstamp);
    }
}


DataUnitLineupInfoGather::DataUnitLineupInfoGather() {

}

DataUnitLineupInfoGather::~DataUnitLineupInfoGather() {

}

DataUnitLineupInfoGather::DataUnitLineupInfoGather(int c, int fs_ms, int threshold_ms, int i_num) :
        DataUnit(c, fs_ms, threshold_ms, i_num) {

}

int DataUnitLineupInfoGather::FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task,
                                           bool isFront) {
    //1寻找最大帧数
    int maxPkgs = 0;
    int maxPkgsIndex;
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        if (iter.size() > maxPkgs) {
            maxPkgs = iter.size();
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
        i_queue_vector.at(maxPkgsIndex).front(refer);
    } else {
        i_queue_vector.at(maxPkgsIndex).back(refer);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                  << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = refer.timstamp;
    }
    std::cout << "取同一帧时,标定时间戳为:" << to_string(curTimestamp) << endl;
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
        bool isFind = false;
        do {
            if (iter.empty()) {
                std::cout << "第" << to_string(i) << "路数据为空" << endl;
                isFind = true;
            } else {
                IType refer;
                if (iter.front(refer)) {
                    if (uint64_t(refer.timstamp) < leftTimestamp) {
                        //在左值外
                        if (iter.size() == 1) {
                            //取用
                            IType cur;
                            if (iter.pop(cur)) {
                                //记录当前路的时间戳
                                xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                oneFrame.insert(oneFrame.begin() + i, cur);
                                std::cout << "第" << to_string(i) << "路时间戳较旧但只有1帧，保留:"
                                          << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                          << endl;
                                isFind = true;
                            }
                        } else {
                            iter.pop(refer);
                            std::cout << "第" << to_string(i) << "路时间戳较旧，舍弃:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << endl;
                        }
                    } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                               (uint64_t(refer.timstamp) <= rightTimestamp)) {
                        //在范围内
                        IType cur;
                        if (iter.pop(cur)) {
                            //记录当前路的时间戳
                            xRoadTimestamp[i] = cur.timstamp;
                            //记录路口编号
                            crossID = cur.crossCode;
                            //将当前路的所有信息缓存入对应的索引
                            oneFrame.insert(oneFrame.begin() + i, cur);
                            std::cout << "第" << to_string(i) << "路时间戳在范围内，保留:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << ":" << to_string(rightTimestamp) << endl;
                            isFind = true;
                        }
                    } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                        //在右值外
                        std::cout << "第" << to_string(i) << "路时间戳较新，保留:"
                                  << to_string(uint64_t(refer.timstamp)) << ":" << to_string(rightTimestamp)
                                  << endl;
                        isFind = true;
                    }
                }
            }
        } while (!isFind);
    }
    //调用后续的处理
    if (task != nullptr) {
        task(this);
    }

    return 0;
}

void DataUnitLineupInfoGather::TaskProcessOneFrame(DataUnitLineupInfoGather *dataUnit) {
    typedef LineupInfo IType;
    typedef LineupInfoGather OType;


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
    if (!dataUnit->o_queue.push(item)) {
        Error("%s队列已满，未存入数据 timestamp:%lu", __FUNCTION__, item.timstamp);
    } else {
        Info("%s数据存入,timestamp:%lu", __FUNCTION__, item.timstamp);
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
        auto iter = i_queue_vector.at(i);
        if (iter.size() > maxPkgs) {
            maxPkgs = iter.size();
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
        i_queue_vector.at(maxPkgsIndex).front(refer);
    } else {
        i_queue_vector.at(maxPkgsIndex).back(refer);
    }
    //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (fs_ms * cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                  << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
        curTimestamp = timestampThreshold;
    } else {
        curTimestamp = refer.timstamp;
    }
    std::cout << "取同一帧时,标定时间戳为:" << to_string(curTimestamp) << endl;
    uint64_t leftTimestamp = curTimestamp - thresholdFrame;
    uint64_t rightTimestamp = curTimestamp + thresholdFrame;

    //3取数
    oneFrame.clear();
    for (int i = 0; i < i_queue_vector.size(); i++) {
        auto iter = i_queue_vector.at(i);
        //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
        bool isFind = false;
        do {
            if (iter.empty()) {
                std::cout << "第" << to_string(i) << "路数据为空" << endl;
                isFind = true;
            } else {
                IType refer;
                if (iter.front(refer)) {
                    if (uint64_t(refer.timstamp) < leftTimestamp) {
                        //在左值外
                        if (iter.size() == 1) {
                            //取用
                            IType cur;
                            if (iter.pop(cur)) {
                                //记录当前路的时间戳
                                xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                crossID = cur.matrixNo;
                                //将当前路的所有信息缓存入对应的索引
                                oneFrame.insert(oneFrame.begin() + i, cur);
                                std::cout << "第" << to_string(i) << "路时间戳较旧但只有1帧，保留:"
                                          << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                          << endl;
                                isFind = true;
                            }
                        } else {
                            iter.pop(refer);
                            std::cout << "第" << to_string(i) << "路时间戳较旧，舍弃:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << endl;
                        }
                    } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                               (uint64_t(refer.timstamp) <= rightTimestamp)) {
                        //在范围内
                        IType cur;
                        if (iter.pop(cur)) {
                            //记录当前路的时间戳
                            xRoadTimestamp[i] = cur.timstamp;
                            //记录路口编号
                            crossID = cur.matrixNo;
                            //将当前路的所有信息缓存入对应的索引
                            oneFrame.insert(oneFrame.begin() + i, cur);
                            std::cout << "第" << to_string(i) << "路时间戳在范围内，保留:"
                                      << to_string(uint64_t(refer.timstamp)) << ":" << to_string(leftTimestamp)
                                      << ":" << to_string(rightTimestamp) << endl;
                            isFind = true;
                        }
                    } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                        //在右值外
                        std::cout << "第" << to_string(i) << "路时间戳较新，保留:"
                                  << to_string(uint64_t(refer.timstamp)) << ":" << to_string(rightTimestamp)
                                  << endl;
                        isFind = true;
                    }
                }
            }
        } while (!isFind);
    }
    //调用后续的处理
    if (task != nullptr) {
        task(this, mergeType);
    }

    return 0;
}

void DataUnitFusionData::TaskProcessOneFrame(DataUnitFusionData *dataUnit, DataUnitFusionData::MergeType mergeType) {
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
    switch (mergeType) {
        case DataUnitFusionData::NotMerge: {
            dataUnit->TaskNotMerge(roadDataInSet);
        }
            break;
        case DataUnitFusionData::Merge: {
            dataUnit->TaskMerge(roadDataInSet);
        }
            break;
    }

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

    GetFusionData(mergeData);
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
                content += (to_string(i) + split +
                            to_string(iter1.locationX) + split +
                            to_string(iter1.locationY) + split +
                            to_string(iter1.speedX) + split +
                            to_string(iter1.speedY) + split +
                            to_string(iter1.objID) + split +
                            to_string(iter1.objType));
                content += "\n";
            }
            //存入图片
            string inDataPicFileName =
                    "mergeData/" + to_string(roadDataInSet.timestamp) + "_in_" + to_string(i) + ".jpeg";
            ofstream inDataPicFile;
            inDataPicFile.open(inDataPicFileName);
            if (inDataPicFile.is_open()) {
                inDataPicFile.write(iter.imageData.data(), iter.imageData.size());
                Info("融合数据图片写入");
                inDataPicFile.flush();
                inDataPicFile.close();
            }

        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) content.c_str(), content.size());
            Info("融合数据写入");
            inDataFile.flush();
            inDataFile.close();
        } else {
            Error("打开文件失败:%s", inDataFileName.c_str());
        }
    }


    MergeData mergeData;
    mergeData.timestamp = roadDataInSet.timestamp;
    mergeData.obj.clear();

    mergeData.objInput.roadDataList.reserve(roadDataInSet.roadDataList.size());
    switch (haveOneRoadDataCount) {
        case 0 : {
            Info("全部路都没有数据");
        }
            break;
        case 1 : {
            Info("只有1路有数据,不走融合");
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
            Info("多于1路有数据,走融合");
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
            contentO += (to_string(iter.objID1) + split +
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
            contentO += "\n";
        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) contentO.c_str(), contentO.size());
            inDataFile.flush();
            inDataFile.close();
        } else {
//          Error("打开文件失败:%s", inDataFileNameO.c_str());
        }
    }

    GetFusionData(mergeData);
}

int DataUnitFusionData::GetFusionData(MergeData mergeData) {
    const int INF = 0x7FFFFFFF;
    FusionData fusionData;
    fusionData.timstamp = mergeData.timestamp;
    fusionData.crossID = this->crossID;
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

        fusionData.lstObjTarget.push_back(objMix);
    }

    bool isSendPicData = true;
    if (isSendPicData) {
        fusionData.isHasImage = 1;
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
            fusionData.lstVideos.push_back(videoData);
        }
    } else {
        fusionData.isHasImage = 0;
        fusionData.lstVideos.resize(0);
    }

    if (o_queue.push(fusionData)) {
        Info("存入融合数据成功,timestamp:%lu", (uint64_t) fusionData.timstamp);
    } else {
        Info("存入融合数据失败,timestamp:%lu", (uint64_t) fusionData.timstamp);
    }

    return 0;
}

