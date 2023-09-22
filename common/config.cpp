//
// Created by lining on 2023/4/23.
//

#include "config.h"
#include <fstream>
#include <glog/logging.h>

LocalConfig localConfig;
int fixrPort = 9977;
string savePath = "/mnt/mnt_hd/save/";

SignalControl *signalControl = nullptr;

mutex conns_mutex;
vector<void *> conns;

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

        try {
            json::decode(content, out);
        } catch (std::exception &e) {
            LOG(ERROR) << e.what();
            return -1;
        }
        return 0;
    }
}

int g_net_status_total = 0;
int g_net_status_success = 0;

void CalNetStatus(int s) {
    if (g_net_status_total == 0xffffffff) {
        g_net_status_total = 0;
        g_net_status_success = 0;
    } else {
        g_net_status_total++;
        if (s == 0) {
            g_net_status_success++;
        }
    }
}
