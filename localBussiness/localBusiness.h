//
// Created by lining on 10/10/22.
//

#ifndef _LOCALBUSINESS_H
#define _LOCALBUSINESS_H

#include <server/FusionServer.h>
#include <client/FusionClient.h>
#include <map>
#include <functional>
#include "monitor/PacketLoss.hpp"

class LocalBusiness {
public:
    bool isRun = false;
    std::map<string, FusionServer *> serverList;
    std::map<string, FusionClient *> clientList;
public:
    LocalBusiness();

    ~LocalBusiness();

    void AddServer(string name, int port);

    void AddClient(string name, string cloudIp, int cloudPort);

    void Run();

public:
    Timer timerKeep;
    Timer timerFusionData;
    Timer timerTrafficFlowGather;
    Timer timerCrossTrafficJamAlarm;
    Timer timerIntersectionOverflowAlarm;
    Timer timerInWatchData_1_3_4;
    Timer timerInWatchData_2;
    Timer timerStopLinePassData;
    Timer timerAbnormalStopData;
    Timer timerLongDistanceOnSolidLineAlarm;
    Timer timerHumanData;

    Timer timerCreateFusionData;

    void StartTimerTask();

    void StopTimerTaskAll();

private:
    /**
    * 查看服务端和客户端状态，并执行断线重连
    * @param p
    */
    static void Task_Keep(void *p);

    static int SendDataUnitO(LocalBusiness *local, string msgType, Pkg pkg, uint64_t timestamp, bool isSave = false);

    static void Task_CrossTrafficJamAlarm(void *p);

    static void Task_IntersectionOverflowAlarm(void *p);

    static void Task_TrafficFlowGather(void *p);

    static void Task_FusionData(void *p);

    static void Task_InWatchData_1_3_4(void *p);

    static void Task_InWatchData_2(void *p);

    static void Task_StopLinePassData(void *p);

    static void Task_HumanData(void *p);

    static void Task_AbnormalStopData(void *p);

    static void Task_LongDistanceOnSolidLineAlarm(void *p);

    //造数据的线程
    static void Task_CreateFusionData(void *p);

    static void Task_CreateCrossTrafficJamAlarm(void *p);

    static void Task_CreateTrafficFlowGather(void *p);
};


#endif //_LOCALBUSINESS_H
