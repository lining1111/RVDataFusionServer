//
// Created by lining on 2023/4/23.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <stdint.h>
#include "data/Data.h"
#include "signalControl/SignalControl.h"


typedef struct {
    bool isSendPIC;
    int roadNum;
    int mergeMode;
    bool isSaveOutObj = false;
    bool isSaveInObj = false;
    bool isSaveOutJSON = false;
    AlgorithmParam algorithmParam;
} LocalConfig;

typedef enum {
    MergerMode_RV = 0,
    MergerMode_Radar = 1,
    MergerMode_Video = 2,
} MergerMode;

extern LocalConfig localConfig;

extern int fixrPort;

extern string savePath;

extern int getAlgorithmParam(string file,AlgorithmParam &out);

extern SignalControl *signalControl;

#endif //CONFIG_H
