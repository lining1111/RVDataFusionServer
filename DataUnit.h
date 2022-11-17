//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"

using namespace std;

template<typename T, int c, int threshold>
class DataUnit {
public:
    int sn = 0;
    string crossID;
    int capacity;//容量
    Queue<T> m_queue;//数据队列
    int thresholdFrame;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    uint64_t xRoadTimestamp[4] = {0, 0, 0, 0};

public:
    DataUnit() {
        capacity = c;
        thresholdFrame = threshold;
        m_queue.setMax(c);
    }

    ~DataUnit() {

    }
};


#endif //_DATAUNIT_H
