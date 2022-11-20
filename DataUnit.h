//
// Created by lining on 11/17/22.
//

#ifndef _DATAUNIT_H
#define _DATAUNIT_H

#include "Queue.h"
#include <mutex>

using namespace std;

template<typename I, typename O>
class DataUnit {
public:
    mutex mtx;
    int sn = 0;
    string crossID;
    vector<Queue<I>> i_queue_vector;
    Queue<O> o_queue;//数据队列
    int fs_ms;
    int cap;
    int thresholdFrame = 10;//时间戳相差门限，单位ms
    uint64_t curTimestamp = 0;
    vector<uint64_t> xRoadTimestamp;
public:
    vector<I> oneFrame;//寻找同一时间戳的数据集

public:
    DataUnit() {

    }

    ~DataUnit() {

    }

    void init(int c, int fs_ms, int threshold_ms, int i_num) {
        this->fs_ms = fs_ms;
        this->cap = c;
        thresholdFrame = threshold_ms;
        i_queue_vector.reserve(i_num);
        for (int i = 0; i < i_queue_vector.size(); i++) {
            auto iter = i_queue_vector.at(i);
            iter.setMax(2 * c);
        }
        o_queue.setMax(c);

        oneFrame.reserve(i_num);
        xRoadTimestamp.resize(i_num);
        for (auto &iter: xRoadTimestamp) {
            iter = 0;
        }
    }


};

#endif //_DATAUNIT_H
