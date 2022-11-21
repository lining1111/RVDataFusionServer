//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"
#include "common/common.h"
#include <mutex>
#include "merge/merge.h"
#include "merge/mergeStruct.h"

using namespace common;

using namespace std;

template<typename I, typename O>
class DataUnit {
public:
    mutex mtx;
    int sn = 0;
    string crossID;
    vector<Queue<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int fs_ms;
    int cap;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;
public:
    vector<I> oneFrame;//寻找同一时间戳的数据集

public:
    DataUnit() {

    }

    DataUnit(int c, int fs_ms, int threshold_ms, int i_num) {
        init(c, fs_ms, threshold_ms, i_num);
    }

    ~DataUnit() {

    }

    void init(int c, int fs_ms, int threshold_ms, int i_num) {
        this->fs_ms = fs_ms;
        this->cap = c;
        thresholdFrame = threshold_ms;
        i_queue_vector.reserve(i_num);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = i_queue_vector.at(i);
            iter.setMax(2 * c);
        }
        o_queue.setMax(c);

        oneFrame.reserve(i_num);
        xRoadTimestamp.resize(i_num);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
    }
};

//车辆轨迹
class DataUnitMultiViewCarTracks : public DataUnit<MultiViewCarTrack, MultiViewCarTracks> {
public:
    DataUnitMultiViewCarTracks();

    ~DataUnitMultiViewCarTracks();

    DataUnitMultiViewCarTracks(int c, int fs_ms, int threshold_ms, int i_num);

    typedef MultiViewCarTrack IType;
    typedef MultiViewCarTracks OType;

    typedef void(*Task)(DataUnitMultiViewCarTracks *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static void TaskProcessOneFrame(DataUnitMultiViewCarTracks *dataUnit);
};

//车流量统计
class DataUnitTrafficFlows : public DataUnit<TrafficFlow, TrafficFlows> {
public:
    DataUnitTrafficFlows();

    ~DataUnitTrafficFlows();

    DataUnitTrafficFlows(int c, int fs_ms, int threshold_ms, int i_num);

    typedef TrafficFlow IType;
    typedef TrafficFlows OType;

    typedef void(*Task)(DataUnitTrafficFlows *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static void TaskProcessOneFrame(DataUnitTrafficFlows *dataUnit);
};

//交叉路口堵塞报警
class DataUnitCrossTrafficJamAlarm : public DataUnit<CrossTrafficJamAlarm, CrossTrafficJamAlarm> {
public:
    DataUnitCrossTrafficJamAlarm();

    ~DataUnitCrossTrafficJamAlarm();

    DataUnitCrossTrafficJamAlarm(int c, int fs_ms, int threshold_ms, int i_num);

    typedef CrossTrafficJamAlarm IType;
    typedef CrossTrafficJamAlarm OType;

    typedef void(*Task)(DataUnitCrossTrafficJamAlarm *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static void TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit);
};

//排队长度等信息
class DataUnitLineupInfoGather : public DataUnit<LineupInfo, LineupInfoGather> {
public:
    DataUnitLineupInfoGather();

    ~DataUnitLineupInfoGather();

    DataUnitLineupInfoGather(int c, int fs_ms, int threshold_ms, int i_num);

    typedef LineupInfo IType;
    typedef LineupInfoGather OType;

    typedef void(*Task)(DataUnitLineupInfoGather *);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, bool isFront = true);

    static void TaskProcessOneFrame(DataUnitLineupInfoGather *dataUnit);
};

//监控数据
class DataUnitFusionData : public DataUnit<WatchData, FusionData> {
public:
    DataUnitFusionData();

    ~DataUnitFusionData();

    DataUnitFusionData(int c, int fs_ms, int threshold_ms, int i_num);

    typedef WatchData IType;
    typedef FusionData OType;
    typedef enum {
        NotMerge,
        Merge,
    } MergeType;

    typedef void(*Task)(DataUnitFusionData *, MergeType);

    int FindOneFrame(unsigned int cache, uint64_t toCacheCha, Task task, MergeType mergeType, bool isFront = true);

    static void TaskProcessOneFrame(DataUnitFusionData *dataUnit, DataUnitFusionData::MergeType mergeType);

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
