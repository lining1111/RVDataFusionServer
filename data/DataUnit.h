//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"
#include "common/common.h"
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <functional>
#include "os/timeTask.hpp"
#include <glog/logging.h>
#include "merge/merge.h"
#include "merge/mergeStruct.h"

using namespace common;

using namespace std;
using namespace os;

//自动帧率调节 记录每次的帧率，然后每10次计算一次平均数，计入平均帧率，并影响数据集合的帧率和时间戳相差门限
/*class DyFrame {
private:
    int *fs = nullptr;
    int *thresholdFrame = nullptr;
    uint64_t preFrameTimestamp = 0;//上一帧的时间戳，在未记录开始前，为0
    atomic_int fsTotal = {0};
    atomic_int fsCount = {0};
    mutex mtx;
public:
    DyFrame(int *p_fs, int *p_thresholdFrame) {
        this->fs = p_fs;
        this->thresholdFrame = p_thresholdFrame;
    }

    ~DyFrame() {

    }

    void UpDate(uint64_t timestamp) {
        std::unique_lock<std::mutex> lock(mtx);
        //preFrameTimestamp 为 0 刚开始
        if (preFrameTimestamp != 0) {
            int64_t fs = timestamp - preFrameTimestamp;
            if (fs < 0) {
                fs = -fs;
            }
            fsTotal += fs;
            fsCount++;
            if (fsCount >= 10) {
                *this->fs = (fsTotal / fsCount);
                *this->thresholdFrame = (fsTotal / fsCount);
                VLOG(3) << "count " << fsCount << " update fs:" << *this->fs;
                fsTotal.store(0);
                fsCount.store(0);
            }
        } else {
            preFrameTimestamp = timestamp;
            VLOG(3) << "DyFrame start:" << preFrameTimestamp;
        }
    }
};
*/

template<typename I, typename O>
class DataUnit {
public:
    int sn = 0;
    vector<Queue<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int cache = 0;
    int i_maxSizeIndex = 0;
    int fs_i;
    int cap;
    int numI = 0;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;

//    DyFrame *dyFrame = nullptr;
public:
    void *owner = nullptr;
public:
    typedef I IType;
    typedef O OType;
    vector<I> oneFrame;//寻找同一时间戳的数据集
    pthread_mutex_t oneFrameMutex = PTHREAD_MUTEX_INITIALIZER;
public:
    DataUnit() : cap(30), thresholdFrame(100), fs_i(100), numI(4), cache(3) {
        i_queue_vector.resize(numI);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = i_queue_vector.at(i);
            iter.setMax(2 * cap);
        }
        o_queue.setMax(numI);

        oneFrame.resize(numI);

        xRoadTimestamp.resize(numI);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
    }

    DataUnit(int c, int threshold_ms, int i_num, int i_cache, void *owner) {
        init(c, threshold_ms, i_num, i_cache, owner);
    }

    ~DataUnit() {
        delete timerBusiness;
//        delete dyFrame;
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
            auto iter = i_queue_vector.at(i);
            iter.setMax(2 * c);
        }
        o_queue.setMax(c);

        oneFrame.resize(i_num);

        xRoadTimestamp.resize(i_num);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
//        dyFrame = new DyFrame(&this->fs_i, &this->thresholdFrame);
    }

    bool frontI(I &i, int index) {
        try {
            return i_queue_vector.at(index).front(i);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
        }
    }

    bool backI(I &i, int index) {
        try {
            return i_queue_vector.at(index).back(i);
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return false;
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

    bool popI(I &i, int index) {
        try {
            return i_queue_vector.at(index).pop(i);
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

    int emptyI(int index) {
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

    int emptyO() {
        try {
            return o_queue.empty();
        } catch (const std::exception &e) {
            cout << __FUNCTION__ << e.what() << endl;
            return true;
        }
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

    DataUnitTrafficFlowGather(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

    static void FindOneFrame(DataUnitTrafficFlowGather *dataUnit, uint64_t toCacheCha, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

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

    DataUnitCrossTrafficJamAlarm(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

    static void FindOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit, uint64_t toCacheCha, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitCrossTrafficJamAlarm *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit);
};

//监控数据
class DataUnitFusionData : public DataUnit<WatchData, FusionData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitFusionData() {

    }

    ~DataUnitFusionData() {

    }

    DataUnitFusionData(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

    typedef enum {
        NotMerge,
        Merge,
    } MergeType;

    static void
    FindOneFrame(DataUnitFusionData *dataUnit, uint64_t toCacheCha, MergeType mergeType, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitFusionData *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitFusionData *dataUnit, DataUnitFusionData::MergeType mergeType);

    typedef struct {
        string hardCode;//图像对应的设备编号
        vector<OBJECT_INFO_T> listObjs;
        string imageData;
    } RoadData;//路口数据，包含目标集合、路口设备编号

    typedef struct {
        uint64_t timestamp;
        vector<RoadData> roadDataList;//数组成员是每一路的目标集合
    } RoadDataInSet;//同一帧多个路口的数据集合

    typedef struct {
        double timestamp;
        vector<OBJECT_INFO_NEW> objOutput;//融合输出量
        RoadDataInSet objInput;//融合输入量
    } MergeData;

    queue<vector<OBJECT_INFO_NEW>> cacheAlgorithmMerge;//一直存的是上帧和上上帧的输出
    //算法用的参量
    double repateX = 10;//fix 不变
    double widthX = 21.3;//跟路口有关
    double widthY = 20;//跟路口有关
    double Xmax = 300;//固定不变
    double Ymax = 300;//固定不变
    double gatetx = 30;//跟路口有关
    double gatety = 30;//跟路口有关
    double gatex = 10;//跟路口有关
    double gatey = 10;//跟路口有关
//    bool time_flag = true;
    int angle_value = -1000;


    int TaskNotMerge(RoadDataInSet roadDataInSet);

    int TaskMerge(RoadDataInSet roadDataInSet);

    int GetFusionData(MergeData mergeData);
};


//路口溢出报警
class DataUnitIntersectionOverflowAlarm : public DataUnit<IntersectionOverflowAlarm, IntersectionOverflowAlarm> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitIntersectionOverflowAlarm() {

    }

    ~DataUnitIntersectionOverflowAlarm() {

    }

    DataUnitIntersectionOverflowAlarm(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

    static void FindOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit, uint64_t toCacheCha, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitIntersectionOverflowAlarm *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit);
};

class DataUnitInWatchData_1_3_4 : public DataUnit<InWatchData_1_3_4, InWatchData_1_3_4> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitInWatchData_1_3_4() {

    }

    ~DataUnitInWatchData_1_3_4() {

    }

    DataUnitInWatchData_1_3_4(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

};

class DataUnitInWatchData_2 : public DataUnit<InWatchData_2, InWatchData_2> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitInWatchData_2() {

    }

    ~DataUnitInWatchData_2() {

    }

    DataUnitInWatchData_2(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);

};

class DataUnitStopLinePassData : public DataUnit<StopLinePassData, StopLinePassData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitStopLinePassData() {

    }

    ~DataUnitStopLinePassData() {

    }

    DataUnitStopLinePassData(int c, int threshold_ms, int i_num, int cache, void *owner);

    void init(int c, int threshold_ms, int i_num, int cache, void *owner);

    static void task(void *local);
};

#endif //_DATAUNIT_H
