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
#include "merge/merge.h"
#include "merge/mergeStruct.h"

using namespace common;

using namespace std;

template<typename I, typename O>
class DataUnit {
public:
    int sn = 0;
    string crossID;
    vector<Queue<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int cache = 0;
    int cap;
    int numI = 0;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;
    atomic<int> i_maxSize = {0};//输入数组内单个通道内的最大缓存数
    atomic<int> i_maxSizeIndex = {0};
    mutex mtx_i;
    condition_variable cv_i_task;

    mutex mtx_o;
    condition_variable cv_o_task;
public:
    //动态帧率相关
#define FSCOUNT 100
    vector<atomic<int>> vector_timeStart_i;
    vector<atomic<int>> vector_count_i;
    vector<atomic<int>> vector_fs_i;
    atomic<int> fs_i = {0};

    atomic<int> timeStart_o = {0};
    atomic<int> count_o = {0};
    atomic<int> fs_o = {0};

    void UpdateFSI(int index) {
        //先更新对应路的信息
        if (vector_timeStart_i.at(index) == 0) {
            auto now = std::chrono::system_clock::now();
            vector_timeStart_i.at(index) = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
        }
        vector_count_i.at(index)++;
        if (vector_count_i.at(index) == FSCOUNT) {
            //计算当前路的帧率
            auto now = std::chrono::system_clock::now();
            vector_fs_i.at(index) = ((std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count() - vector_timeStart_i.at(index)) / FSCOUNT);

            //临时变量清零
            vector_timeStart_i.at(index) = 0;
            vector_count_i.at(index) = 0;
            //计算一次加权帧率
            int count = 0;
            int sum = 0;
            for (int i = 0; i < vector_fs_i.size(); i++) {
                if (vector_fs_i.at(i) != 0) {
                    sum += vector_fs_i.at(i);
                    count++;
                }
            }
            fs_i = sum / count;
        }
    }

    void UpdateFSO() {
        if (timeStart_o == 0) {
            auto now = std::chrono::system_clock::now();
            timeStart_o = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
        }
        count_o++;
        if (count_o == FSCOUNT) {
            auto now = std::chrono::system_clock::now();
            fs_o = ((std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count() - timeStart_o) / FSCOUNT);
            //临时变量清零
            timeStart_o = 0;
            count_o = 0;
        }
    }


public:
    typedef I IType;
    typedef O OType;
    vector<I> oneFrame;//寻找同一时间戳的数据集

public:
    DataUnit() : cap(30), thresholdFrame(100), numI(4), cache(3) {
        i_queue_vector.resize(4);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = i_queue_vector.at(i);
            iter.setMax(2 * 30);
        }
        o_queue.setMax(4);

        oneFrame.resize(4);

        xRoadTimestamp.resize(4);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }

        vector_timeStart_i.resize(numI);
        for (auto &iter:vector_timeStart_i) {
            iter = 0;
        }
        vector_count_i.resize(numI);
        for (auto &iter:vector_count_i) {
            iter = 0;
        }
        vector_fs_i.resize(numI);
        for (auto &iter:vector_fs_i) {
            iter = 0;
        }
    }

    DataUnit(int c, int threshold_ms, int i_num, int i_cache) {
        init(c, threshold_ms, i_num, i_cache);
    }

    ~DataUnit() {

    }

public:

    void init(int c, int threshold_ms, int i_num, int cache) {
        this->cap = c;
        this->numI = i_num;
        this->thresholdFrame = threshold_ms;
        this->cache = cache;
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

        vector_timeStart_i.resize(numI);
        vector_count_i.resize(numI);
        vector_fs_i.resize(numI);
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

    void updateIMaxSizeForTask(int index) {
        unique_lock<mutex> lck(mtx_i);

        //更新最大缓存信息
        if (i_queue_vector.at(index).size() > i_maxSize) {
            i_maxSize = i_queue_vector.at(index).size();
            i_maxSizeIndex = index;
        }

        if (i_maxSize >= cache) {
            cv_i_task.notify_one();
        }
    }

    void runTask(std::function<void()> task) {
        unique_lock<mutex> lck(mtx_i);
        while (i_maxSize < cache) {
            cv_i_task.wait(lck);
        }

        //执行相应的流程
        task();
        //更新最大缓存信息
        int maxSize = 0;
        int maxSizeIndex = 0;
        for (int i = 0; i < i_queue_vector.size(); i++) {
            if (i_queue_vector.at(i).size() > maxSize) {
                maxSize = i_queue_vector.at(i).size();
                maxSizeIndex = i;
            }
        }
        i_maxSize = maxSize;
        i_maxSizeIndex = maxSizeIndex;

    }
};

//车辆轨迹
class DataUnitCarTrackGather : public DataUnit<CarTrack, CarTrackGather> {
public:
    int saveCount = 0;// 测试存包用
    DataUnitCarTrackGather();

    ~DataUnitCarTrackGather();

    DataUnitCarTrackGather(int c, int threshold_ms, int i_num, int cache);

    static void FindOneFrame(DataUnitCarTrackGather *dataUnit, uint64_t toCacheCha, bool isFront);

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

    DataUnitTrafficFlowGather(int c, int threshold_ms, int i_num, int cache);

    static void FindOneFrame(DataUnitTrafficFlowGather *dataUnit, uint64_t toCacheCha, bool isFront = true);

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

    DataUnitCrossTrafficJamAlarm(int c, int threshold_ms, int i_num, int cache);

    static void FindOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit, uint64_t toCacheCha, bool isFront = true);

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

    DataUnitLineupInfoGather(int c, int threshold_ms, int i_num, int cache);

    static void FindOneFrame(DataUnitLineupInfoGather *dataUnit, uint64_t toCacheCha, bool isFront = true);

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

    DataUnitFusionData(int c, int threshold_ms, int i_num, int cache);

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
