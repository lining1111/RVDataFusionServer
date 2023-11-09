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


int PkgProcessFun_CmdResponse(void *p, string content);

int PkgProcessFun_CmdControl(void *p, string content);

int PkgProcessFun_CmdHeartBeat(void *p, string content);

int PkgProcessFun_CmdFusionData(void *p, string content);

int PkgProcessFun_CmdCrossTrafficJamAlarm(void *p, string content);

int PkgProcessFun_CmdIntersectionOverflowAlarm(void *p, string content);

int PkgProcessFun_CmdTrafficFlowGather(void *p, string content);

int PkgProcessFun_CmdInWatchData_1_3_4(void *p, string content);

int PkgProcessFun_CmdInWatchData_2(void *p, string content);

int PkgProcessFun_StopLinePassData(void *p, string content);

int PkgProcessFun_AbnormalStopData(void *p, string content);

int PkgProcessFun_LongDistanceOnSolidLineAlarm(void *p, string content);

int PkgProcessFun_HumanData(void *p, string content);

int PkgProcessFun_HumanLitPoleData(void *p, string content);

int PkgProcessFun_0xf1(void *p, string content);

int PkgProcessFun_0xf2(void *p, string content);

int PkgProcessFun_0xf3(void *p, string content);

int PkgProcessFun_TrafficDetectorStatus(void *p, string content);

typedef int (*PkgProcessFun)(void *p, string content);

extern map<CmdType, PkgProcessFun> PkgProcessMap;
#endif //PROC_H
