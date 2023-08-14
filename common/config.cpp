//
// Created by lining on 2023/4/23.
//

#include "config.h"
#include <fstream>

LocalConfig localConfig;
int fixrPort = 9977;
string savePath = "/mnt/mnt_hd/save/";

SignalControl *signalControl = nullptr;

int getAlgorithmParam(string file, AlgorithmParam &out) {
    //打开文件
    std::ifstream ifs;
    ifs.open(file);
    if (!ifs.is_open()) {
        return -1;
    } else {
        std::stringstream buf;
        buf << ifs.rdbuf();
        std::string content(buf.str());
        ifs.close();
        Json::Reader reader;
        Json::Value in;
        if (!reader.parse(content, in, false)) {
            printf("json parse err");
            return -1;
        }
        if (out.JsonUnmarshal(in)) {
            return 0;
        } else {
            return -1;
        }
    }
}