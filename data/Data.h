//
// Created by lining on 2023/2/27.
//

#ifndef DATA_H
#define DATA_H

#include "os/nonCopyable.hpp"
#include "DataUnit.h"
#include <mutex>

class Data : public NonCopyable {
public:
    std::mutex mtx;
    static Data *m_pInstance;
    bool isRun = false;
public:

    //各个数据
    //---------------监控数据相关---------//
    DataUnitFusionData dataUnitFusionData{30, 100, 4, 10, this};//100ms一帧
    //---------------路口交通流向相关--------//
    DataUnitTrafficFlowGather dataUnitTrafficFlowGather{30, 1000, 4, 10, this};//1000ms一帧
    //------交叉口堵塞报警------//
    DataUnitCrossTrafficJamAlarm dataUnitCrossTrafficJamAlarm{30, 1000, 4, 10, this};//1000ms一帧
    //------路口溢出报警上传-----//
    DataUnitIntersectionOverflowAlarm dataUnitIntersectionOverflowAlarm{30, 1000, 4, 10, this};//1000ms一帧
    //-----进口监控数据上传----//
    DataUnitInWatchData_1_3_4 dataUnitInWatchData_1_3_4{30, 1000, 4, 0, this};
    DataUnitInWatchData_2 dataUnitInWatchData_2{30, 1000, 4, 0, this};
    //-----停止线过车数据----//
    DataUnitStopLinePassData dataUnitStopLinePassData{30, 1000, 4, 0, this};
    //-----3516相机警报数据----//
    DataUnitCamera3516Alarm dataUnitCamera3516Alarm{30, 1000, 4, 0, this};
public:
    //本地参数
    //FusionData相关
    bool isMerge = true;
    vector<int> roadDirection = {
            North,//北
            East,//东
            South,//南
            West,//西
    };
    //无特定顺序存入的数据
    vector<string> unOrder;//记录已传入的路号，方便将数据存入对应的数据集合内的输入量
    //本地属性
    string db = "CLParking.db";
    string matrixNo = "0123456789";
    string plateId;
public:
    static Data *instance();

    ~Data() {
        isRun = false;
    }

    int FindIndexInUnOrder(const string in);

private:
    int getMatrixNo();

    int getPlatId();
};

#endif //DATA_H
