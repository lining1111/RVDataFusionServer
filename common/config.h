//
// Created by lining on 2023/4/23.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <stdint.h>
#include <mutex>
#include "signalControl/SignalControl.h"
#include "lib/AlgorithmParam.h"


typedef struct {
    bool isSendPIC;
    int roadNum = 8;
    int mergeMode;
    bool isSaveInObj = false;
    bool isSaveOutObj = false;
    bool isSendCloud = true;
    bool isSaveOtherJson = false;
    bool isSaveRVJson = false;
    string algorithmParamFile;
    AlgorithmParam algorithmParam;
    vector<string> msgType;
    int mode = 0;
} LocalConfig;

typedef enum {
    MergerMode_RV = 0,
    MergerMode_Radar = 1,
    MergerMode_Video = 2,
} MergerMode;

extern LocalConfig localConfig;

bool isShowMsgType(string msgType);

extern int fixrPort;

extern string savePath;

extern int getAlgorithmParam(string file,AlgorithmParam &out);

extern SignalControl *signalControl;

extern mutex conns_mutex;
extern vector<void *> conns;

//发送实时数据的统计
extern int g_net_status_total;
extern int g_net_status_success;

void CalNetStatus(int s);

extern string homePath;

#endif //CONFIG_H
