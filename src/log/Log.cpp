//
// Created by lining on 2/15/22.
//

#include <iostream>
#include <dirent.h>
#include "log/Log.h"

using namespace std;

namespace z_log {
    zlog_category_t *m_zlog_category = nullptr;
    const string configPath = "./log.conf";//zlog 配置存储地方 可用 zlog-chk-conf 命令验证文件的语法
    const string configName = "log_conf";//配置文件中[rules]conf名称
    const string logPath = "z_log";//日志输入目录
    bool isInit = false;//是否已初始化

    int init() {
        if (isInit) {
            return 0;
        }

        int rc = zlog_init(configPath.c_str());
        if (rc == -1) {
            cout << "zlog init fail" << endl;
            zlog_fini();
            return -1;
        } else {
            //检查日志目录是否存在，不存在则创建
            DIR *dir = opendir(logPath.c_str());
            if (dir == nullptr) {
                string cmd = "mkdir -p " + logPath;
                system(cmd.c_str());
            }
            m_zlog_category = zlog_get_category(configName.c_str());
            isInit = true;
            return 0;
        }
    }

    int finish() {
        zlog_fini();
        isInit = false;
        return 0;
    }

}