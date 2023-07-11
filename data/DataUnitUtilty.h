//
// Created by lining on 2023/7/5.
//

#ifndef RVDATAFUSIONSERVER_DATAUNITUTILTY_H
#define RVDATAFUSIONSERVER_DATAUNITUTILTY_H

#include <string>
#include <vector>
#include "merge/merge.h"
using namespace std;

void SaveDataIn(vector<OBJECT_INFO_T> data, uint64_t timestamp, string path);

void SaveDataOut(vector<OBJECT_INFO_NEW> data, uint64_t timestamp, string path);


#endif //RVDATAFUSIONSERVER_DATAUNITUTILTY_H
