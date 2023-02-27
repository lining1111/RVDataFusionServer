//
// Created by lining on 2/15/22.
// RV数据 接收服务端
//

#ifndef _FUSIONSERVER_H
#define _FUSIONSERVER_H

#include <cstdint>
#include <ctime>
#include <vector>
#include <sys/epoll.h>
#include <future>
#include "server/ClientInfo.h"
#include "net/tcpServer/TcpServer.hpp"
#include "os/timeTask.hpp"

using namespace std;
using namespace os;

class FusionServer : public TcpServer<ClientInfo> {
public:
    const timeval checkStatusTimeval = {3, 0};//连续3s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};
public:
    typedef map<string, Timer *> TimerTasks;
    vector<pthread_t> localBusinessThreadHandle;
    TimerTasks timerTasks;

public:
    FusionServer(int port, bool isMerge, int cliNum = 4);

    ~FusionServer();

public:
    /**
     * 打开
     * @return 0：success -1：fail
     */
    int Open();

    /**
     * 运行服务端线程
     * @return 0:success -1:fail
     */
    int Run();

    /**
     * 关闭
     * @return 0：success -1：fail
     */
    int Close();

private:

    static int StartLocalBusiness(void *pServer);

    static int StopLocalBusiness(void *pServer);

    void addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task);

    void deleteTimerTask(string name);

    void StopTimerTaskAll();


};

#endif //_FUSIONSERVER_H
