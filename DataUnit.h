//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"

using namespace std;

template<typename I, typename O, int c, int fs_ms, int threshold_ms, int i_num>
class DataUnit {
public:
    int sn = 0;
    string crossID;
    int fs;
    int capacity;//容量
    int iNum;//输入量有几路
    vector<Queue<I, 2 * c>> i_queue_vector;
    Queue<O, c> o_queue;//数据队列
    int thresholdFrame;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    uint64_t xRoadTimestamp[4] = {0, 0, 0, 0};
public:
    vector<I> oneFrame;//寻找同一时间戳的数据集

public:
    DataUnit() {
        capacity = c;
        fs = fs_ms;
        thresholdFrame = threshold_ms;
        iNum = i_num;
        i_queue_vector.reserve(i_num);
        oneFrame.reserve(i_num);
    }

    ~DataUnit() {

    }

};


#endif //_DATAUNIT_H
