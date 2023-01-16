//
// Created by lining on 1/16/23.
//

#include <chrono>
#include "util.h"

FSDynamic::FSDynamic() {
    timeStart = 0;
    count = 0;
    fs = 0;
}

FSDynamic::~FSDynamic() {

}

FSDynamic::FSDynamic(int fs, int maxCount) : fs(fs), maxCount(maxCount) {
    timeStart = 0;
    count = 0;
}

void FSDynamic::Update() {
    if (timeStart == 0) {
        auto now = std::chrono::system_clock::now();
        timeStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
    }
    count++;
    if (count == maxCount) {
        //update fs
        auto now = std::chrono::system_clock::now();
        fs = ((std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count() - timeStart) / maxCount);
        //临时变量清零
        timeStart = 0;
        count = 0;
    }
}
