//
// Created by lining on 2023/5/27.
//

#ifndef DATAUNITFUSIONDATA_H
#define DATAUNITFUSIONDATA_H

#include "DataUnit.h"

using namespace common;

//监控数据
class DataUnitFusionData : public DataUnit<WatchData, FusionData> {
public:
    int saveCount = 0;// 测试存包用

    DataUnitFusionData() {

    }

    ~DataUnitFusionData() {

    }

    void taskI();

    int haveDataRoadNum = 0;
    uint64_t totalCost = 0;
    uint64_t algorithmCost = 0;

    typedef enum {
        NotMerge,
        Merge,
    } MergeType;

    static void FindOneFrame(DataUnitFusionData *dataUnit, MergeType mergeType, int offset);

    int TaskProcessOneFrame(DataUnitFusionData::MergeType mergeType);

    typedef struct {
        string hardCode;//图像对应的设备编号
        vector<OBJECT_INFO_T> listObjs;
        string imageData;
        int direction;
        uint64_t timestamp;
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


    // 新版多路融合数据输入！
    int GateContinueNum = 6;//连续6次都匹配上，转为稳定航迹
    int GateFailNum = 3;//连续3次都失配，删除航迹
    double gate = 8;
    bool time_flag = false;
    vector<double> FixTheta = {0, 0, 0};
    vector<double> RoadTheta = {0.0, 90.0 / 180 * 3.1415926, -90.0 / 180 * 3.1415926};


    int TaskNotMerge(RoadDataInSet roadDataInSet);

    int TaskMerge(RoadDataInSet roadDataInSet);

    int GetFusionData(MergeData mergeData);

    void Summary(MergeData *mergeData, FusionData *fusionData);

    void taskO();
};

#endif //DATAUNITFUSIONDATA_H
