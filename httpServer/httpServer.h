//
// Created by lining on 10/10/22.
//

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H
#include "global.h"

#include <string>
#include <vector>

using namespace std;

typedef struct{
   string ip;
   int direction;
}FusionClientInfo;

typedef struct {
    int clientNum;
    vector<FusionClientInfo> fusionClientInfos;
}FusionServerInfo;


typedef struct {
    bool isRun;
    bool isFusionServerRun;
    bool isMerge;
    bool isMultiViewServerRun;
    bool isClientRun;
}LocalConnectInfo;

int HttpServerInit(int port,Local *local);

int JsonMarshalFusionServerInfo(FusionServerInfo fusionServerInfo,string &out);
int JsonMarshalLocalConnectInfo(LocalConnectInfo localConnectInfo, string &out);
#endif //_HTTPSERVER_H
