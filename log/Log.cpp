//
// Created by lining on 2/15/22.
//

#include <iostream>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <future>
#include "log/Log.h"

using namespace std;

namespace z_log {
    zlog_category_t *m_zlog_category = nullptr;
    const string configPath = "./log.conf";//zlog 配置存储地方 可用 zlog-chk-conf 命令验证文件的语法
    const string configName = "log_conf";//配置文件中[rules]conf名称
    const string logPath = "/mnt/mnt_hd/log";//日志输入目录
//    const string logPath = "./log/";//日志输入目录
    bool isInit = false;//是否已初始化

    string localProgramName;

    static void GetDirFiles(string path, vector<string> &array) {
        DIR *dir;
        struct dirent *ptr;
        char base[1000];
        if ((dir = opendir(path.c_str())) == nullptr) {
            printf("can not open dir %s\n", path.c_str());
            return;
        } else {
            while ((ptr = readdir(dir)) != nullptr) {
                if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0)) {
//            printf("it is dir\n");
                } else if (ptr->d_type == DT_REG) {
                    string name = ptr->d_name;
                    array.push_back(name);
                }
            }
            closedir(dir);
        }
    }

    static bool startsWith(const std::string str, const std::string prefix) {
        return (str.rfind(prefix, 0) == 0);
    }

    static bool endsWith(const std::string str, const std::string suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }

        return (str.rfind(suffix) == (str.length() - suffix.length()));
    }

    static void regularCleanLogFile(int seconds) {
        if (seconds <= 0) {
            return;
        }
        printf("开启日志定时清理任务\n");
        while (isInit) {
            sleep(5);
            //获取符合条件的日志文件 {ProgramName-*.log}
            vector<string> files;
            files.clear();
            GetDirFiles(logPath, files);
            vector<string> logs;
            logs.clear();
            for (auto &iter:files) {
                if (startsWith(iter, localProgramName) && endsWith(iter, ".log")) {
                    logs.push_back(iter);
                }
            }
            if (!logs.empty()) {
                //比较文件的创建日志和现在的时间
                time_t now;
                time(&now);
                struct stat buf;
                for (auto &iter:logs) {
                    string fulPath = logPath + "/" + iter;
                    if (stat(fulPath.c_str(), &buf) == 0) {
                        if ((now - buf.st_ctime) > seconds) {
                            if (remove(fulPath.c_str()) == 0) {
                                printf("log file:%s create time:%s,now:%s,keepSecond:%d s,delete",
                                       asctime(localtime(&buf.st_ctime)), asctime(localtime(&now)), seconds);
                            }
                        }
                    }
                }
            }
        }
        printf("关闭日志定时清理任务\n");
    }

    int init(string programName, int keepSeconds) {
        localProgramName = programName;
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

            //开启定时清理任务
            if (keepSeconds > 0) {
                std::async(std::launch::async, regularCleanLogFile, keepSeconds);
            }

            return 0;
        }
    }


    int finish() {
        zlog_fini();
        isInit = false;
        return 0;
    }

}