//
// Created by lining on 1/16/23.
//

#ifndef _UTIL_H
#define _UTIL_H

#include <atomic>

class FSDynamic{
public:
    std::atomic<int> timeStart;
    std::atomic<int> count;
    std::atomic<int> fs;
    int maxCount = 100;
public:

    FSDynamic();
    ~FSDynamic();
    FSDynamic(int fs, int maxCount = 100);
    void Update();
};


#endif //_UTIL_H
