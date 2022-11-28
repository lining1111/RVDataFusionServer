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
    typedef map<string, Timer*> TimerTasks;
public:
    bool isRun = false;
    std::map<string, FusionServer *> serverList;
    std::map<string, FusionClient *> clientList;
    TimerTasks timerTasks;
public:
    LocalBusiness();

    ~LocalBusiness();

    void AddServer(string name, int port, bool isMerge);

    void AddClient(string name, string cloudIp, int cloudPort, void *super);

    void Run();

public:
    void addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task);

    void deleteTimerTask(string name);

    void StartTimerTask();

    void StopTimerTaskAll();

private:
    /**
    * 查看服务端和客户端状态，并执行断线重连
    * @param p
    */
    static void Task_Keep(void *p);

    static void Task_MultiViewCarTracks(void *p);

    static void Task_CrossTrafficJamAlarm(void *p);

    static void Task_LineupInfoGather(void *p);

    static void Task_TrafficFlows(void *p);

    static void Task_FusionData(void *p);

};


#endif //_LOCALBUSINESS_H
