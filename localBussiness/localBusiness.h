//
// Created by lining on 10/10/22.
//

#ifndef _LOCALBUSINESS_H
#define _LOCALBUSINESS_H

#include <server/FusionServer.h>
#include <client/FusionClient.h>
#include <map>
#include <functional>
#include "os/nonCopyable.hpp"

class LocalBusiness : public NonCopyable {
public:
    std::mutex mtx;
    static LocalBusiness *m_pInstance;

    bool isRun = false;
    std::map<string, FusionServer *> serverList;
    std::map<string, FusionClient *> clientList;
public:
    static LocalBusiness *instance();

    ~LocalBusiness(){
        StopTimerTaskAll();
        isRun = false;
        for (auto iter = clientList.begin(); iter != clientList.end();) {
            delete iter->second;
            iter = clientList.erase(iter);
        }
        for (auto iter = serverList.begin(); iter != serverList.end();) {
            delete iter->second;
            iter = serverList.erase(iter);
        }
    };


    void AddServer(string name, int port);

    void AddClient(string name, string cloudIp, int cloudPort);

    void Run();

public:
    Timer timerKeep;

    Timer timerCreateFusionData;
    Timer timerCreateTrafficFlowGather;
    Timer timerCreateAbnormalStopData;
    Timer timerCreateLongDistanceOnSolidLineAlarm;
    Timer timerCreateHumanData;

    void StartTimerTask();

    void StopTimerTaskAll();

    static int SendDataUnitO(FusionClient *client, string msgType, Pkg pkg);
private:
    /**
    * 查看服务端和客户端状态，并执行断线重连
    * @param p
    */
    static void Task_Keep(void *p);


    //造数据的线程
    static void Task_CreateFusionData(void *p);

    static void Task_CreateCrossTrafficJamAlarm(void *p);

    static void Task_CreateTrafficFlowGather(void *p);

    static void Task_CreateAbnormalStopData(void *p);

    static void Task_CreateLongDistanceOnSolidLineAlarm(void *p);

    static void Task_CreateHumanData(void *p);
};


#endif //_LOCALBUSINESS_H
