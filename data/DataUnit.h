//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"
#include "Vector.h"
#include "common/common.h"
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <functional>
#include "os/timeTask.hpp"
#include <glog/logging.h>
#include "merge/mergeStruct.h"

using namespace common;
using namespace std;
using namespace os;

#define TaskTimeval 10 //任务开启周期
template<typename I, typename O>
class DataUnit {
public:
    int sn = 0;
    vector<Vector<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int cache = 0;
    int i_maxSizeIndex = -1;
    int taskSearchCount = 0;//未找到下个标定缓存帧时+1,直到此值*寻找周期大于设定指时，更新i_maxSizeIndex为下一个

    void taskSearchCountReset() {
        taskSearchCount = 0;
    }

    void i_maxSizeIndexNext() {
        //开一个满缓存的路数开始，总能找到一个满缓存的路

        i_maxSizeIndex++;

        if (i_maxSizeIndex > i_queue_vector.size()) {
            i_maxSizeIndex = 0;
        }
        taskSearchCountReset();

    }

    int fs_i;
    int cap;
    int numI = 0;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;
public:
    void *owner = nullptr;
public:
    typedef I IType;
    typedef O OType;
    vector<I> oneFrame;//寻找同一时间戳的数据集
    pthread_mutex_t oneFrameMutex = PTHREAD_MUTEX_INITIALIZER;
    //预先不知道顺序插入队列时使用的
    vector<string> unOrder;

public:
    DataUnit() : cap(30), thresholdFrame(100), fs_i(100), numI(4), cache(3) {
        i_queue_vector.resize(numI);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = &i_queue_vector.at(i);
            iter->setMax(cap);
        }
        o_queue.setMax(cap);

        oneFrame.resize(numI);

        xRoadTimestamp.resize(numI);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
        unOrder.resize(numI);
    }

    DataUnit(int c, int threshold_ms, int i_num, int i_cache, void *owner) {
        init(c, threshold_ms, i_num, i_cache, owner);
    }

    ~DataUnit() {
        delete timerBusiness;
    }

public:

    Timer *timerBusiness;
    string timerBusinessName;

    void init(int c, int fs, int i_num, int cache, void *owner) {
        this->cap = c;
        this->numI = i_num;
        this->thresholdFrame = fs;
        this->fs_i = fs;
        this->cache = cache;
        this->owner = owner;
        i_queue_vector.resize(i_num);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = &i_queue_vector.at(i);
            iter->setMax(cap);
        }

        o_queue.setMax(cap);

        oneFrame.resize(numI);

        xRoadTimestamp.resize(numI);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
        unOrder.resize(numI);
    }

    bool getIOffset(I &i, int index, int offset) {
        try {
            return i_queue_vector.at(index).getIndex(i, offset);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    void eraseBeginI(int index) {
        try {
            i_queue_vector.at(index).eraseBegin();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
        }
    }

    bool pushI(I i, int index) {
        try {
            return i_queue_vector.at(index).push(i);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    int sizeI(int index) {
        try {
            return i_queue_vector.at(index).size();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return 0;
        }
    }

    bool emptyI(int index) {
        try {
            return i_queue_vector.at(index).empty();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return true;
        }
    }

    bool frontO(O &o) {
        try {
            return o_queue.front(o);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    bool backO(O &o) {
        try {
            return o_queue.back(o);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    bool pushO(O o) {
        try {
            return o_queue.push(o);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    bool popO(O &o) {
        try {
            return o_queue.pop(o);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    int sizeO() {
        try {
            return o_queue.size();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return 0;
        }
    }

    bool emptyO() {
        try {
            return o_queue.empty();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return true;
        }
    }

    void clearI(int index) {
        if (index < i_queue_vector.size()) {
            i_queue_vector.at(index).clear();
        }
    }

    void clearO() {
        o_queue.clear();
    }

    int FindIndexInUnOrder(const string in) {
//        printf("in :%s\n", in.c_str());
        int index = -1;
        //首先遍历是否已经存在
        int alreadyExistIndex = -1;
        for (int i = 0; i < unOrder.size(); i++) {
            auto &iter = unOrder.at(i);
//            printf("iter :%s\n", iter.c_str());
            if (iter == in) {
                alreadyExistIndex = i;
                break;
            }
        }
//        printf("alreadyExistIndex:%d\n", alreadyExistIndex);
        if (alreadyExistIndex >= 0) {
            index = alreadyExistIndex;
        } else {
            //不存在就新加
            for (int i = 0; i < unOrder.size(); i++) {
                auto &iter = unOrder.at(i);
                if (iter.empty()) {
                    iter = in;
//                    printf("iter1 :%s\n", iter.c_str());
                    index = i;
                    break;
                }
            }
        }

        return index;
    }

    void runTask(std::function<void()> task) {
        pthread_mutex_lock(&oneFrameMutex);
        int maxSize = 0;
        for (int i = 0; i < i_queue_vector.size(); i++) {
            if (i_queue_vector.at(i).size() > maxSize) {
                maxSize = i_queue_vector.at(i).size();
                i_maxSizeIndex = i;
            }
        }
        if (maxSize > cache) {
            //执行相应的流程
            task();
        }

        pthread_mutex_unlock(&oneFrameMutex);
    }

public:
    //与寻找同一帧标定帧相关
    uint64_t timestampStore = 0;//存的上一次的时间戳，如果这次的和上次的一样，但是时间戳又比设定的阈值小，则不进行后面的操作

};

//车辆轨迹
//class DataUnitCarTrackGather : public DataUnit<CarTrack, CarTrackGather> {
//public:
//    int saveCount = 0;// 测试存包用
//    DataUnitCarTrackGather();
//
//    ~DataUnitCarTrackGather();
//
//    DataUnitCarTrackGather(int c, int threshold_ms, int i_num, int cache);
//
//    static void FindOneFrame(DataUnitCarTrackGather *dataUnit, uint64_t toCacheCha, bool isFront);
//
//    static int ThreadGetDataInRange(DataUnitCarTrackGather *dataUnit,
//                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);
//
//    static int TaskProcessOneFrame(DataUnitCarTrackGather *dataUnit);
//};


//车流量统计
class DataUnitTrafficFlowGather : public DataUnit<TrafficFlow, TrafficFlowGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitTrafficFlowGather() {

    }

    ~DataUnitTrafficFlowGather() {

    }

    DataUnitTrafficFlowGather(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);

    static void FindOneFrame(DataUnitTrafficFlowGather *dataUnit, int offset);

    static int ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit, int index, int offset,
                                    uint64_t curTimestamp);

    static int TaskProcessOneFrame(DataUnitTrafficFlowGather *dataUnit);
};

//交叉路口堵塞报警
class DataUnitCrossTrafficJamAlarm : public DataUnit<CrossTrafficJamAlarm, CrossTrafficJamAlarm> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitCrossTrafficJamAlarm() {

    }

    ~DataUnitCrossTrafficJamAlarm() {

    }

    DataUnitCrossTrafficJamAlarm(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);

};


//路口溢出报警
class DataUnitIntersectionOverflowAlarm : public DataUnit<IntersectionOverflowAlarm, IntersectionOverflowAlarm> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitIntersectionOverflowAlarm() {

    }

    ~DataUnitIntersectionOverflowAlarm() {

    }

    DataUnitIntersectionOverflowAlarm(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);

};

class DataUnitInWatchData_1_3_4 : public DataUnit<InWatchData_1_3_4, InWatchData_1_3_4> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitInWatchData_1_3_4() {

    }

    ~DataUnitInWatchData_1_3_4() {

    }

    DataUnitInWatchData_1_3_4(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);

};

class DataUnitInWatchData_2 : public DataUnit<InWatchData_2, InWatchData_2> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitInWatchData_2() {

    }

    ~DataUnitInWatchData_2() {

    }

    DataUnitInWatchData_2(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);

};

class DataUnitStopLinePassData : public DataUnit<StopLinePassData, StopLinePassData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitStopLinePassData() {

    }

    ~DataUnitStopLinePassData() {

    }

    DataUnitStopLinePassData(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);
};

//异常停车报警
class DataUnitAbnormalStopData : public DataUnit<AbnormalStopData, AbnormalStopData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitAbnormalStopData() {

    }

    ~DataUnitAbnormalStopData() {

    }

    DataUnitAbnormalStopData(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);
};

//长距离压实线报警
class DataUnitLongDistanceOnSolidLineAlarm
        : public DataUnit<LongDistanceOnSolidLineAlarm, LongDistanceOnSolidLineAlarm> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitLongDistanceOnSolidLineAlarm() {

    }

    ~DataUnitLongDistanceOnSolidLineAlarm() {

    }

    DataUnitLongDistanceOnSolidLineAlarm(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);
};

//行人数据
class DataUnitHumanData : public DataUnit<HumanData, HumanDataGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitHumanData() {

    }

    ~DataUnitHumanData() {

    }

    DataUnitHumanData(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);
};


#endif //_DATAUNIT_H
