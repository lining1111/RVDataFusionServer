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

    typedef struct {
        uint64_t last = 0;//上一帧的时间戳
        uint64_t now;//当前的时间戳
    } History;
    typedef struct {
        std::vector<History> in;//接收路的历史记录
        History out;//发送路的历史记录
    } DataUnitHistory;

    //各个数据
    //---------------监控数据相关---------//
    DataUnitHistory dataUnitHistoryFusionData;
    DataUnitFusionData dataUnitFusionData{30, 100, 4, 10, this};//100ms一帧
    //---------------路口交通流向相关--------//
    DataUnitHistory dataUnitHistoryTrafficFlowGather;
    DataUnitTrafficFlowGather dataUnitTrafficFlowGather{30, 1000, 4, 10, this};//1000ms一帧
    //------交叉口堵塞报警------//
    DataUnitHistory dataUnitHistoryCrossTrafficJamAlarm;
    DataUnitCrossTrafficJamAlarm dataUnitCrossTrafficJamAlarm{30, 1000, 4, 10, this};//1000ms一帧
    //------路口溢出报警上传-----//
    DataUnitHistory dataUnitHistoryIntersectionOverflowAlarm;
    DataUnitIntersectionOverflowAlarm dataUnitIntersectionOverflowAlarm{30, 1000, 4, 10, this};//1000ms一帧
    //-----进口监控数据上传----//
    DataUnitHistory dataUnitHistoryInWatchData_1_3_4;
    DataUnitInWatchData_1_3_4 dataUnitInWatchData_1_3_4{30, 1000, 4, 0, this};
    DataUnitHistory dataUnitHistoryInWatchData_2;
    DataUnitInWatchData_2 dataUnitInWatchData_2{30, 1000, 4, 0, this};
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

    /**
     * 通过从dataUnitXX获取的时间戳历史数据dataUnitHistoryXX，进行前后对比，打印帧率信息
     * @param local
     * @param interval
     * @return
     */
    static int startDataUnitHistoryPrint(Data *local, int interval);
};

#endif //DATA_H
