//
// Created by lining on 2023/4/23.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <stdint.h>
#include "data/Data.h"

typedef struct {
    std::string ip;
    uint16_t port;
    bool isEnable;
} ConfigEnable;

typedef struct {
    std::vector<ConfigEnable> isSendPIC;
    int roadNum;
    int mergeMode;
} LocalConfig;

typedef enum {
    MergerMode_RV = 0,
    MergerMode_Radar = 1,
    MergerMode_Video = 2,
} MergerMode;

extern LocalConfig localConfig;

extern int fixrPort;

#endif //CONFIG_H
