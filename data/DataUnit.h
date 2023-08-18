//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.hpp"
#include "Vector.hpp"
#include "common/common.h"
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <functional>
#include "os/timeTask.hpp"
#include <glog/logging.h>
#include <iomanip>
#include "merge/mergeStruct.h"

using namespace common;
using namespace std;
using namespace os;

#define TaskTimeval 10 //任务开启周期

template<typename I, typename O>
class DataUnit {
public:
    string name;
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
    std::mutex *oneFrameMutex = nullptr;
    //预先不知道顺序插入队列时使用的
    vector<string> unOrder;

public:
    DataUnit() {
        if (oneFrameMutex == nullptr) {
            oneFrameMutex = new std::mutex();
        }
    }

    DataUnit(int c, int threshold_ms, int i_num, int i_cache, void *owner) {
        init(c, threshold_ms, i_num, i_cache, owner);
    }

    ~DataUnit() {
        delete timerBusiness;
        delete oneFrameMutex;
    }

public:

    Timer *timerBusiness;
    string timerBusinessName;

    void init(int c, int fs, int i_num, int cache, void *owner) {
        if (oneFrameMutex == nullptr) {
            oneFrameMutex = new std::mutex();
        }
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
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    void eraseBeginI(int index) {
        try {
            i_queue_vector.at(index).eraseBegin();
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
        }
    }

    bool pushI(I i, int index) {
        try {
            return i_queue_vector.at(index).push(i);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    int sizeI(int index) {
        try {
            return i_queue_vector.at(index).size();
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return 0;
        }
    }

    bool emptyI(int index) {
        try {
            return i_queue_vector.at(index).empty();
        } catch (const std::exception &e) {
            LOG(WARNING) << e.what();
            return true;
        }
    }

    bool frontO(O &o) {
        try {
            return o_queue.front(o);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    bool backO(O &o) {
        try {
            return o_queue.back(o);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    bool pushO(O o) {
        try {
            return o_queue.push(o);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    bool popO(O &o) {
        try {
            return o_queue.pop(o);
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return false;
        }
    }

    int sizeO() {
        try {
            return o_queue.size();
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
            return 0;
        }
    }

    bool emptyO() {
        try {
            return o_queue.empty();
        } catch (const std::exception &e) {
            std::cout << e.what() << std::endl;
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
        std::unique_lock<std::mutex> lock(*oneFrameMutex);
        int maxSize = 0;
        int maxSizeIndex = 0;
        for (int i = 0; i < i_queue_vector.size(); i++) {
            if (i_queue_vector.at(i).size() > maxSize) {
                maxSize = i_queue_vector.at(i).size();
                maxSizeIndex = i;
            }
        }
//        VLOG(3) << name << " maxSize:" << maxSize << " maxSizeIndex:" << maxSizeIndex << " cache :" << cache;
        if (maxSize >= cache) {
            if (i_maxSizeIndex == -1) {
                //只有在第一次它为数据默认值-1的时候才执行赋值
                i_maxSizeIndex = maxSizeIndex;
            }
            //执行相应的流程
            task();
        }
    }

public:
    //业务1 与寻找同一帧标定帧相关
    uint64_t timestampStore = 0;//存的上一次的时间戳，如果这次的和上次的一样，但是时间戳又比设定的阈值小，则不进行后面的操作

    static void FindOneFrame(DataUnit *dataUnit, int offset) {
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

        std::time_t t(dataUnit->curTimestamp / 1000);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%F %T");

        VLOG(3) << dataUnit->name << " 取同一帧时,标定时间戳为:" << dataUnit->curTimestamp << " " << ss.str();

        uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
        uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

        //2取数
        vector<IType>().swap(dataUnit->oneFrame);
        dataUnit->oneFrame.resize(dataUnit->numI);
        for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
            dataUnit->ThreadGetDataInRange(i, dataUnit->curTimestamp);
        }

        //打印下每一路取到的时间戳
        for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
            auto iter = dataUnit->oneFrame.at(i);
            if (!iter.oprNum.empty()) {
                VLOG(3) << dataUnit->name << " 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
            }
        }
        //调用后续的处理(用户自定义内容)

    }

    int ThreadGetDataInRange(int index, uint64_t curTimestamp) {
        //找到时间戳在范围内的帧
        if (emptyI(index)) {
            VLOG(3) << name << " 第" << index << "路数据为空";
        } else {
            for (int i = 0; i < sizeI(index); i++) {
                IType refer;
                if (getIOffset(refer, index, i)) {
                    if (abs(((long long) refer.timestamp - (long long) curTimestamp)) < fs_i) {
                        //在范围内
                        //记录当前路的时间戳
                        xRoadTimestamp[index] = (uint64_t) refer.timestamp;
                        //将当前路的所有信息缓存入对应的索引
                        oneFrame[index] = refer;
                        VLOG(3) << name << " 第" << index << "路时间戳在范围内，取出来:" << (uint64_t) refer.timestamp;
                        break;
                    }
                }
            }
        }

        return index;
    }

    //业务2 与透传相关
    static void TransparentTransmission(DataUnit *dataUnit) {
        for (auto &iter: dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.getIndex(cur, 0);
                iter.eraseBegin();
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(2) << dataUnit->name << " 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(2) << dataUnit->name << " 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }
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

    int TaskProcessOneFrame();
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

    static void specialBusiness(DataUnitHumanData *dataUnit);
};


//行人灯杆数据
class DataUnitHumanLitPoleData : public DataUnit<HumanLitPoleData, HumanLitPoleData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitHumanLitPoleData() {

    }

    ~DataUnitHumanLitPoleData() {

    }

    DataUnitHumanLitPoleData(int c, int fs, int i_num, int cache, void *owner);

    void init(int c, int fs, int i_num, int cache, void *owner);

    static void task(void *local);
};

#endif //_DATAUNIT_H
