//
// Created by lining on 11/20/22.
//

#include <iostream>
#include "FindOneFrame.h"

namespace FindOneFrame {
    int
    FindMultiViewCarTracks(DataUnit<common::MultiViewCarTrack, common::MultiViewCarTracks> &dataUnit, unsigned int cache,
                           uint64_t toCacheCha, PMultiViewCarTracks task, bool isFront) {
       typedef common::MultiViewCarTrack IType;
       typedef common::MultiViewCarTrack OType;
        //1寻找最大帧数
        int maxPkgs = 0;
        int maxPkgsIndex;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
            dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer);
        } else {
            dataUnit.i_queue_vector.at(maxPkgsIndex).back(refer);
        }
        //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
        auto now = std::chrono::system_clock::now();
        uint64_t timestampThreshold =
                (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
                 (dataUnit.fs_ms * cache) - toCacheCha);
        if (uint64_t(refer.timstamp) < timestampThreshold) {
            std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                      << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
            dataUnit.curTimestamp = timestampThreshold;
        } else {
            dataUnit.curTimestamp = refer.timstamp;
        }
        std::cout << "取同一帧时,标定时间戳为:" << to_string(dataUnit.curTimestamp) << endl;
        uint64_t leftTimestamp = dataUnit.curTimestamp - dataUnit.thresholdFrame;
        uint64_t rightTimestamp = dataUnit.curTimestamp + dataUnit.thresholdFrame;

        //3取数
        dataUnit.oneFrame.clear();
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                    //记录路口编号
                                    dataUnit.crossID = cur.crossCode;
                                    //将当前路的所有信息缓存入对应的索引
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
                                dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                dataUnit.crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
            task(dataUnit);
        }

        return 0;
    }

    int FindTrafficFlows(DataUnit<common::TrafficFlow, common::TrafficFlows> &dataUnit, unsigned int cache,
                         uint64_t toCacheCha,
                         PTrafficFlows task, bool isFront) {
        typedef common::TrafficFlow IType;
        typedef common::TrafficFlows OType;
        //1寻找最大帧数
        int maxPkgs = 0;
        int maxPkgsIndex;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
            dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer);
        } else {
            dataUnit.i_queue_vector.at(maxPkgsIndex).back(refer);
        }
        //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
        auto now = std::chrono::system_clock::now();
        uint64_t timestampThreshold =
                (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
                 (dataUnit.fs_ms * cache) - toCacheCha);
        if (uint64_t(refer.timstamp) < timestampThreshold) {
            std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                      << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
            dataUnit.curTimestamp = timestampThreshold;
        } else {
            dataUnit.curTimestamp = refer.timstamp;
        }
        std::cout << "取同一帧时,标定时间戳为:" << to_string(dataUnit.curTimestamp) << endl;
        uint64_t leftTimestamp = dataUnit.curTimestamp - dataUnit.thresholdFrame;
        uint64_t rightTimestamp = dataUnit.curTimestamp + dataUnit.thresholdFrame;

        //3取数
        dataUnit.oneFrame.clear();
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                    //记录路口编号
                                    dataUnit.crossID = cur.crossCode;
                                    //将当前路的所有信息缓存入对应的索引
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
                                dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                dataUnit.crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
            task(dataUnit);
        }

        return 0;
    }

    int FindCrossTrafficJamAlarm(DataUnit<common::CrossTrafficJamAlarm, common::CrossTrafficJamAlarm> &dataUnit,
                                 unsigned int cache, uint64_t toCacheCha, PCrossTrafficJamAlarm task, bool isFront) {
        typedef common::CrossTrafficJamAlarm IType;
        typedef common::CrossTrafficJamAlarm OType;
        //1寻找最大帧数
        int maxPkgs = 0;
        int maxPkgsIndex;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
            dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer);
        } else {
            dataUnit.i_queue_vector.at(maxPkgsIndex).back(refer);
        }
        //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
        auto now = std::chrono::system_clock::now();
        uint64_t timestampThreshold =
                (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
                 (dataUnit.fs_ms * cache) - toCacheCha);
        if (uint64_t(refer.timstamp) < timestampThreshold) {
            std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                      << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
            dataUnit.curTimestamp = timestampThreshold;
        } else {
            dataUnit.curTimestamp = refer.timstamp;
        }
        std::cout << "取同一帧时,标定时间戳为:" << to_string(dataUnit.curTimestamp) << endl;
        uint64_t leftTimestamp = dataUnit.curTimestamp - dataUnit.thresholdFrame;
        uint64_t rightTimestamp = dataUnit.curTimestamp + dataUnit.thresholdFrame;

        //3取数
        dataUnit.oneFrame.clear();
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                    //记录路口编号
                                    dataUnit.crossID = cur.crossCode;
                                    //将当前路的所有信息缓存入对应的索引
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
                                dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                dataUnit.crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
            task(dataUnit);
        }

        return 0;
    }

    int FindLineupInfoGather(DataUnit<common::LineupInfo, common::LineupInfoGather> &dataUnit, unsigned int cache,
                             uint64_t toCacheCha, PLineupInfoGather task, bool isFront) {
        typedef common::LineupInfo IType;
        typedef common::LineupInfoGather OType;
        //1寻找最大帧数
        int maxPkgs = 0;
        int maxPkgsIndex;
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
            dataUnit.i_queue_vector.at(maxPkgsIndex).front(refer);
        } else {
            dataUnit.i_queue_vector.at(maxPkgsIndex).back(refer);
        }
        //2.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
        auto now = std::chrono::system_clock::now();
        uint64_t timestampThreshold =
                (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
                 (dataUnit.fs_ms * cache) - toCacheCha);
        if (uint64_t(refer.timstamp) < timestampThreshold) {
            std::cout << "当前时间戳:" << to_string((uint64_t) refer.timstamp)
                      << "小于缓存阈值:" << to_string(timestampThreshold) << endl;
            dataUnit.curTimestamp = timestampThreshold;
        } else {
            dataUnit.curTimestamp = refer.timstamp;
        }
        std::cout << "取同一帧时,标定时间戳为:" << to_string(dataUnit.curTimestamp) << endl;
        uint64_t leftTimestamp = dataUnit.curTimestamp - dataUnit.thresholdFrame;
        uint64_t rightTimestamp = dataUnit.curTimestamp + dataUnit.thresholdFrame;

        //3取数
        dataUnit.oneFrame.clear();
        for (int i = 0; i < dataUnit.i_queue_vector.size(); i++) {
            auto iter = dataUnit.i_queue_vector.at(i);
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
                                    dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                    //记录路口编号
                                    dataUnit.crossID = cur.crossCode;
                                    //将当前路的所有信息缓存入对应的索引
                                    dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
                                dataUnit.xRoadTimestamp[i] = cur.timstamp;
                                //记录路口编号
                                dataUnit.crossID = cur.crossCode;
                                //将当前路的所有信息缓存入对应的索引
                                dataUnit.oneFrame.insert(dataUnit.oneFrame.begin() + i, cur);
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
            task(dataUnit);
        }

        return 0;
    }
}