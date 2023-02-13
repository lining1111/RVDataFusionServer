//
// Created by lining on 2023/2/13.
//

#include <dirent.h>
#include <cstring>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include "LogFileCleaner.h"


static void GetDirFiles(std::string path, std::vector<std::string> &array) {
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
                std::string name = ptr->d_name;
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


LogFileCleaner::LogFileCleaner(std::string _name, std::string _logPath, int _period) :
        name(_name), logPath(_logPath), period(_period) {
    isRun = true;
    futureRun = std::async(std::launch::async, run, this);
}

LogFileCleaner::~LogFileCleaner() {
    isRun = false;
    try {
        futureRun.wait();
    } catch (std::future_error e) {
        std::cout << e.what() << std::endl;
    }
}

int LogFileCleaner::run(void *p) {
    if (p == nullptr) {
        return -1;
    }
    auto local = (LogFileCleaner *) p;


    if (local->period <= 0) {
        return -1;
    }
    printf("开启日志定时清理任务\n");
    while (local->isRun) {
        sleep(5);
        //获取符合条件的日志文件 {ProgramName-*.log}
        std::vector<std::string> files;
        files.clear();
        GetDirFiles(local->logPath, files);
        std::vector<std::string> logs;
        logs.clear();
        for (auto &iter:files) {
            if (startsWith(iter, local->name)) {
                logs.push_back(iter);
            }
        }
        if (!logs.empty()) {
            //比较文件的创建日志和现在的时间
            time_t now;
            time(&now);
            struct stat buf;
            for (auto &iter:logs) {
                std::string fulPath = local->logPath + "/" + iter;
                if (stat(fulPath.c_str(), &buf) == 0) {
                    if ((now - buf.st_ctime) > local->period) {
                        if (remove(fulPath.c_str()) == 0) {
                            printf("log file:%s create time:%s,now:%s,keepSecond:%d s,delete",
                                   asctime(localtime(&buf.st_ctime)), asctime(localtime(&now)), local->period);
                        }
                    }
                }
            }
        }
    }
    printf("关闭日志定时清理任务\n");
}
