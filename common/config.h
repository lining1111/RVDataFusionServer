//
// Created by lining on 2023/4/23.
//

#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
#include <stdint.h>

typedef struct {
    std::string ip;
    uint16_t port;
    bool isEnable;
}ConfigEnable;

typedef struct {
    std::vector<ConfigEnable> isSendPIC;
}LocalConfig;

extern LocalConfig localConfig;



#endif //CONFIG_H
