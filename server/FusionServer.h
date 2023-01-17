//
// Created by lining on 2/15/22.
// RV数据 接收服务端
//

#ifndef _FUSIONSERVER_H
#define _FUSIONSERVER_H

#include <cstdint>
#include <ctime>
#include <vector>
#include <sys/epoll.h>
#include <future>
#include "server/ClientInfo.h"
#include "net/tcpServer/TcpServer.hpp"
#include "merge/merge.h"
#include "DataUnit.h"
#include "os/timeTask.hpp"

using namespace std;
using namespace os;

class FusionServer : public TcpServer<ClientInfo> {
public:
    bool isMerge = true;
    const timeval checkStatusTimeval = {3, 0};//连续3s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};

public:
    typedef map<string, Timer *> TimerTasks;

    vector<pthread_t> localBusinessThreadHandle;
    TimerTasks timerTasks;

    vector<int> roadDirection = {
            North,//北
            East,//东
            South,//南
            West,//西
    };
    vector<string> unOrder;//记录已传入的路号，方便将数据存入对应的数据集合内的输入量
    //---------------监控数据相关---------//
    DataUnitFusionData dataUnitFusionData;
    //---------车辆轨迹---------//
    DataUnitCarTrackGather dataUnitCarTrackGather;
    //---------------路口交通流向相关--------//
    DataUnitTrafficFlowGather dataUnitTrafficFlowGather;
    //------交叉口堵塞报警------//
    DataUnitCrossTrafficJamAlarm dataUnitCrossTrafficJamAlarm;
    //--------排队长度等信息------//
    DataUnitLineupInfoGather dataUnitLineupInfoGather;

    bool isLocalBusinessRun = false;
    //处理线程

    string db = "CLParking.db";
    string matrixNo = "0123456789";
    string plateId;

public:
    FusionServer(int port, bool isMerge, int cliNum = 4);

    ~FusionServer();

private:
    int getMatrixNoFromDb();

    int getPlatId();

public:
    /**
     * 打开
     * @return 0：success -1：fail
     */
    int Open();

    /**
     * 运行服务端线程
     * @return 0:success -1:fail
     */
    int Run();

    /**
     * 关闭
     * @return 0：success -1：fail
     */
    int Close();


    int FindIndexInUnOrder(const string in);

private:

    static int StartLocalBusiness(void *pServer);

    static int StopLocalBusiness(void *pServer);

    void addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task);

    void deleteTimerTask(string name);

    void StopTimerTaskAll();


};

#endif //_FUSIONSERVER_H
