//
// Created by lining on 2023/2/27.
//

#ifndef DATA_H
#define DATA_H

#include "os/nonCopyable.hpp"
#include "DataUnit.h"
#include "DataUnitFusionData.h"
#include <mutex>

class Data : public NonCopyable {
public:
    std::mutex mtx;
    static Data *m_pInstance;
    bool isRun = false;
public:

    //各个数据
    //---------------监控数据相关---------//
    DataUnitFusionData dataUnitFusionData{10, 80, 4, 10, m_pInstance};
    //---------------路口交通流向相关--------//
    DataUnitTrafficFlowGather dataUnitTrafficFlowGather{10, 500, 4, 3, m_pInstance};//1000ms一帧
    //------交叉口堵塞报警------//
    DataUnitCrossTrafficJamAlarm dataUnitCrossTrafficJamAlarm{10, 5, 4, 10, m_pInstance};//1000ms一帧
    //------路口溢出报警上传-----//
    DataUnitIntersectionOverflowAlarm dataUnitIntersectionOverflowAlarm{10, 5, 4, 10, m_pInstance};//1000ms一帧
    //-----进口监控数据上传----//
    DataUnitInWatchData_1_3_4 dataUnitInWatchData_1_3_4{10, 5, 4, 1, m_pInstance};
    DataUnitInWatchData_2 dataUnitInWatchData_2{10, 5, 4, 1, m_pInstance};
    //-----停止线过车数据----//
    DataUnitStopLinePassData dataUnitStopLinePassData{10, 5, 4, 1, m_pInstance};
    //-----异常停车数据-----//
    DataUnitAbnormalStopData dataUnitAbnormalStopData{10, 5, 4, 1, m_pInstance};
    //-----长距离压实线报警----//
    DataUnitLongDistanceOnSolidLineAlarm dataUnitLongDistanceOnSolidLineAlarm{10, 5, 4, 1, m_pInstance};
    //-----行人数据----//
    DataUnitHumanData dataUnitHumanData{10, 5, 4, 1, m_pInstance};
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
    //本地属性
    string db = "CLParking.db";
    string matrixNo = "0123456789";
    string plateId;
public:
    static Data *instance();

    ~Data() {
        isRun = false;
    }

private:
    int getMatrixNo();

    int getPlatId();
};

#endif //DATA_H
