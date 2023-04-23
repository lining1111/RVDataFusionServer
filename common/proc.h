//
// Created by lining on 2023/4/23.
//

#ifndef PROC_H
#define PROC_H

#include <string>
#include <server/ClientInfo.h>

using namespace std;
int PkgProcessFun_CmdResponse(string ip, uint16_t port, string content);
int PkgProcessFun_CmdControl(string ip, uint16_t port, string content);
int PkgProcessFun_CmdHeartBeat(string ip, uint16_t port, string content);
int PkgProcessFun_CmdFusionData(string ip, uint16_t port, string content);
int PkgProcessFun_CmdCrossTrafficJamAlarm(string ip, uint16_t port, string content);
int PkgProcessFun_CmdIntersectionOverflowAlarm(string ip, uint16_t port, string content);
int PkgProcessFun_CmdTrafficFlowGather(string ip, uint16_t port, string content);
int PkgProcessFun_CmdInWatchData_1_3_4(string ip, uint16_t port, string content);
int PkgProcessFun_CmdInWatchData_2(string ip, uint16_t port, string content);
int PkgProcessFun_StopLinePassData(string ip, uint16_t port, string content);
int PkgProcessFun_AbnormalStopData(string ip, uint16_t port, string content);
int PkgProcessFun_LongDistanceOnSolidLineAlarm(string ip, uint16_t port, string content);
int PkgProcessFun_HumanData(string ip, uint16_t port, string content);

typedef int (*PkgProcessFun)(string ip, uint16_t port, string content);

extern map<CmdType, PkgProcessFun> PkgProcessMap;
#endif //PROC_H
