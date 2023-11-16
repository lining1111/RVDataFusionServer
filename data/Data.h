//
// Created by lining on 2023/2/27.
//

#ifndef DATA_H
#define DATA_H

#include "os/nonCopyable.hpp"
#include "DataUnit.h"
#include "DataUnitFusionData.h"
#include <mutex>
#include "ReSendQueue.h"

class Data : public NonCopyable {
public:
    std::mutex mtx;
    static Data *m_pInstance;
    bool isRun = false;
public:
    bool isStartFusion = false;
    //各个数据
    //---------------监控数据相关---------//
    DataUnitFusionData *dataUnitFusionData;
    //---------------路口交通流向相关--------//
    DataUnitTrafficFlowGather *dataUnitTrafficFlowGather;
    //------交叉口堵塞报警------//
    DataUnitCrossTrafficJamAlarm *dataUnitCrossTrafficJamAlarm;
    //------路口溢出报警上传-----//
    DataUnitIntersectionOverflowAlarm *dataUnitIntersectionOverflowAlarm;
    //-----进口监控数据上传----//
    DataUnitInWatchData_1_3_4 *dataUnitInWatchData_1_3_4;
    DataUnitInWatchData_2 *dataUnitInWatchData_2;
    //-----停止线过车数据----//
    DataUnitStopLinePassData *dataUnitStopLinePassData;
    //-----异常停车数据-----//
    DataUnitAbnormalStopData *dataUnitAbnormalStopData;
    //-----长距离压实线报警----//
    DataUnitLongDistanceOnSolidLineAlarm *dataUnitLongDistanceOnSolidLineAlarm;
    //-----行人数据----//
    DataUnitHumanData *dataUnitHumanData;
    //-----行人灯杆数据----//
    DataUnitHumanLitPoleData *dataUnitHumanLitPoleData;
    //-----检测器状态----//
    DataUnitTrafficDetectorStatus *dataUnitTrafficDetectorStatus;

    //-------重发队列------//
    ReSendQueue *reSendQueue;//拥堵、溢出、异常停车 需要加入队列
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

//    vector<int> roadDirection = {
//            East,//东
//            West,//西
//            South,//南
//            North,//北
//            Northeast,//东北
//            Southwest,//西南
//            Southeast,//东南
//            Northwest,//西北
//    };
    //无特定顺序存入的数据
    //本地属性
    string db = "CLParking.db";
    string matrixNo = "0123456789";
    string plateId;
public:
    static Data *instance();

    ~Data() {
        isRun = false;
        delete dataUnitFusionData;
        delete dataUnitTrafficFlowGather;
        delete dataUnitCrossTrafficJamAlarm;
        delete dataUnitIntersectionOverflowAlarm;
        delete dataUnitInWatchData_1_3_4;
        delete dataUnitInWatchData_2;
        delete dataUnitStopLinePassData;
        delete dataUnitAbnormalStopData;
        delete dataUnitLongDistanceOnSolidLineAlarm;
        delete dataUnitHumanData;
        delete dataUnitHumanLitPoleData;
    }

private:
    int getMatrixNo();

    int getPlatId();
};

#endif //DATA_H
