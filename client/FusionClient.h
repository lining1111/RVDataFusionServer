//
// Created by lining on 3/8/22.
//

#ifndef _FUSIONCLIENT_H
#define _FUSIONCLIENT_H

#include <string>
#include <future>
#include <queue>

#include "common/common.h"
#include "ringbuffer/RingBuffer.h"
#include "Queue.h"
#include "monitor/PacketLoss.hpp"
#include "os/timeTask.hpp"
#include "utility/utility.h"

using namespace std;
using namespace common;
using namespace os;

class FusionClient {
public:
    string server_ip;
    unsigned int server_port;
    int sockfd = 0;
    pthread_mutex_t lock_sock = PTHREAD_MUTEX_INITIALIZER;
    RingBuffer *rb = nullptr;
#define RecvSize (1024*512)
    bool isRun = false;//运行标志
    timeval receive_time = {0, 0};

public:
    bool isLocalThreadRun = false;
    std::shared_future<int> futureDump;
    std::shared_future<int> futureProcessRev;
    std::shared_future<int> futureProcessSend;
    long checkStatus_timeval = 5;
    long heartBeatTimeval = 5/*30*/;//应该和server端保持一致

    //接收buffer状态机 Start开始---从接收的数据中寻找帧头开始(开始为特殊字符$)GetHeadStart---找到帧头开始，获取全部帧头信息(一共9字节)GetHead---
    //获取全部帧头信息后，根据帧头信息的帧内容长度信息，获取全部帧内容GetBody---获取完身体后，获取CRC GETCRC---获取完全部帧内容后，回到开始状态Start
    typedef enum {
        Start = 0, GetHeadStart, GetHead, GetBody, GetCRC,
    } RecvStatus;//表示状态机已经达到的状态
private:
    RecvStatus status = Start;
    //用于缓存解包
    PkgHead pkgHead;//分包头
    int bodyLen = 0;//获取分包头后，得到的包长度
    uint8_t *pkgBuffer = nullptr;//分包缓冲
    int index = 0;//分包缓冲的索引
public:
    Queue<Pkg> queuePkg = Queue<Pkg>(1000);;//包消息队列
    Queue<Pkg> queue_send = Queue<Pkg>(1000);
public:
    typedef map<string, Timer *> TimerTasks;
    TimerTasks timerTasks;
public:

    FusionClient(string server_ip, unsigned int server_port);

    ~FusionClient();

public:
    int Open();

    int Run();

    int Close();

public:
    void addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task);

    void deleteTimerTask(string name);

    void StartTimerTask();

    void StopTimerTaskAll();

private:
    static int ThreadDump(void *p);

    static int ThreadProcessRecv(void *p);

    static int ThreadProcessSend(void *p);

public:
    //send to server
    int SendQueue(Pkg pkg);

    int SendBase(Pkg pkg);

};


#endif //_FUSIONCLIENT_H
