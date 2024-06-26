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

typedef int (*PkgProcessFun)(void *p, string content);

extern map<CmdType, PkgProcessFun> PkgProcessMap;
#endif //PROC_H
