//
// Created by lining on 2023/4/23.
//

#ifndef PROC_H
#define PROC_H

#include <string>
#include <vector>
#include <common/common.h>

using namespace std;
using namespace common;

class CacheTimestamp {
private:

    std::vector<uint64_t> dataTimestamps;
    int dataIndex = -1;

public:
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    uint64_t interval = 0;
    bool isSetInterval = false;
    bool isStartTask = false;
public:
    void update(int index, uint64_t timestamp, int caches = 10);

};


int PkgProcessFun_CmdResponse(string ip, string content);

int PkgProcessFun_CmdControl(string ip, string content);

int PkgProcessFun_CmdHeartBeat(string ip, string content);

int PkgProcessFun_CmdFusionData(string ip, string content);

int PkgProcessFun_CmdCrossTrafficJamAlarm(string ip, string content);

int PkgProcessFun_CmdIntersectionOverflowAlarm(string ip, string content);

int PkgProcessFun_CmdTrafficFlowGather(string ip, string content);

int PkgProcessFun_CmdInWatchData_1_3_4(string ip, string content);

int PkgProcessFun_CmdInWatchData_2(string ip, string content);

int PkgProcessFun_StopLinePassData(string ip, string content);

int PkgProcessFun_AbnormalStopData(string ip, string content);

int PkgProcessFun_LongDistanceOnSolidLineAlarm(string ip, string content);

int PkgProcessFun_HumanData(string ip, string content);

int PkgProcessFun_HumanLitPoleData(string ip, string content);

int PkgProcessFun_0xf1(string ip, string content);

int PkgProcessFun_0xf2(string ip, string content);

int PkgProcessFun_0xf3(string ip, string content);

int PkgProcessFun_TrafficDetectorStatus(string ip, string content);

typedef int (*PkgProcessFun)(string ip, string content);

extern map<CmdType, PkgProcessFun> PkgProcessMap;
#endif //PROC_H
