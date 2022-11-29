//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"
#include "common/common.h"
#include <mutex>
#include <iostream>
#include "merge/merge.h"
#include "merge/mergeStruct.h"

using namespace common;

using namespace std;

template<typename I, typename O>
class DataUnit {
public:
//    mutex mtx;
    int sn = 0;
    string crossID;
    vector<Queue<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int fs_ms;
    int cap;
    int numI = 0;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;
public:
    typedef I IType;
    typedef O OType;
    vector<I> oneFrame;//寻找同一时间戳的数据集

public:
    DataUnit() : cap(30), fs_ms(100), thresholdFrame(100), numI(4) {

    }

    DataUnit(int c, int fs_ms, int threshold_ms, int i_num) {
        init(c, fs_ms, threshold_ms, i_num);
    }

    ~DataUnit() {

    }

public:

    void init(int c, int fs_ms, int threshold_ms, int i_num) {
        this->fs_ms = fs_ms;
        this->cap = c;
        this->numI = i_num;
        thresholdFrame = threshold_ms;
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
};

//车辆轨迹
class DataUnitCarTrackGather : public DataUnit<CarTrack, CarTrackGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitCarTrackGather();

    ~DataUnitCarTrackGather();

    DataUnitCarTrackGather(int c, int fs_ms, int threshold_ms, int i_num);

    typedef int(*Task)(DataUnitCarTrackGather *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitCarTrackGather *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitCarTrackGather *dataUnit);
};

//车流量统计
class DataUnitTrafficFlowGather : public DataUnit<TrafficFlow, TrafficFlowGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitTrafficFlowGather();

    ~DataUnitTrafficFlowGather();

    DataUnitTrafficFlowGather(int c, int fs_ms, int threshold_ms, int i_num);

    typedef int(*Task)(DataUnitTrafficFlowGather *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitTrafficFlowGather *dataUnit);
};

//交叉路口堵塞报警
class DataUnitCrossTrafficJamAlarm : public DataUnit<CrossTrafficJamAlarm, CrossTrafficJamAlarm> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitCrossTrafficJamAlarm();

    ~DataUnitCrossTrafficJamAlarm();

    DataUnitCrossTrafficJamAlarm(int c, int fs_ms, int threshold_ms, int i_num);

    typedef int(*Task)(DataUnitCrossTrafficJamAlarm *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitCrossTrafficJamAlarm *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit);
};

//排队长度等信息
class DataUnitLineupInfoGather : public DataUnit<LineupInfo, LineupInfoGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitLineupInfoGather();

    ~DataUnitLineupInfoGather();

    DataUnitLineupInfoGather(int c, int fs_ms, int threshold_ms, int i_num);

    typedef int(*Task)(DataUnitLineupInfoGather *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static int ThreadGetDataInRange(DataUnitLineupInfoGather *dataUnit,
                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp);

    static int TaskProcessOneFrame(DataUnitLineupInfoGather *dataUnit);
};

//监控数据
class DataUnitFusionData : public DataUnit<WatchData, FusionData> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitFusionData();

    ~DataUnitFusionData();

    DataUnitFusionData(int c, int fs_ms, int threshold_ms, int i_num);

    typedef enum {
        NotMerge,
        Merge,
    } MergeType;

    typedef int(*Task)(DataUnitFusionData *, MergeType);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, MergeType mergeType, bool isFront = true);

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
        vector<OBJECT_INFO_NEW> obj;//融合输出量
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

#endif //_DATAUNIT_H
