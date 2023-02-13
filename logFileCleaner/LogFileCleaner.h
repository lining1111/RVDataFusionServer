//
// Created by lining on 2023/2/13.
//

#ifndef LOGFILECLEANER_H
#define LOGFILECLEANER_H

#include <string>
#include <future>

class LogFileCleaner {
private:
    std::string name;
    std::string logPath;
    int period;
    bool isRun = false;
    std::future<int> futureRun;
public:
    LogFileCleaner(std::string _name, std::string _logPath, int _period);

    ~LogFileCleaner();

private:
    static int run(void *p);
};


#endif //RVDATAFUSIONSERVER_LOGFILECLEANER_H
