//
// Created by lining on 2/15/22.
//

#ifndef _CLIENTINFO_H
#define _CLIENTINFO_H

#include "ringBuffer/RingBuffer.h"
#include "common/common.h"
#include <netinet/in.h>
#include <queue>
#include <thread>
#include "common/Queue.h"

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;
using namespace common;
class ClientInfo {
public:

    //接收buffer状态机 Start开始---从接收的数据中寻找帧头开始(开始为特殊字符$)GetHeadStart---找到帧头开始，获取全部帧头信息(一共9字节)GetHead---
    //获取全部帧头信息后，根据帧头信息的帧内容长度信息，获取全部帧内容GetBody---获取完身体后，获取CRC GETCRC---获取完全部帧内容后，回到开始状态Start
    typedef enum {
        Start = 0, GetHeadStart, GetHead, GetBody, GetCRC,
    } RecvStatus;//表示状态机已经达到的状态
#define MAX_RB (1024*1024*64)
public:
    //都是客户端的属性信息
    int msgid = 0;//帧号
    struct sockaddr_in clientAddr;
    int sock;//客户端socket
    int clientType = 0;
    unsigned char extraData[1024 * 8];//特性数据
    timeval receive_time;
    RecvStatus status = Start;
    RingBuffer *rb = nullptr;//接收数据缓存环形buffer
    bool isLive = false;

    //供给服务端使用的变量
    bool needRelease = false;//客户端处理线程在发现sock异常时，向上抛出释放队列内容信号

    //客户端处理线程
    thread threadDump;//接收数据并存入环形buffer

    //GetPkg 生产者 GetPkgContent消费者 通过queue加锁的方式完成传递
    Queue<Pkg> queuePkg;//包消息队列
    const int maxQueuePkg = 600;//最多600个

    //从包队列中依据方法名获取正文结构体，有多少方法名就有多少队列

    //WatchData队列
    Queue<WatchData> queueWatchData;
    const int maxQueueWatchData = 600;//最多600个

    thread threadGetPkg;//将环形buffer内的数据进行分包
    thread threadGetPkgContent;//获取一包内的数据正文

public:
    /**
     * 初始化一个客户端对象
     * @param clientAddr 客户端的地址信息
     * @param client_sock 客户端socket
     * @param rbCapacity 客户端循环buffer的最大容量
     */
    ClientInfo(struct sockaddr_in clientAddr, int client_sock, long long int rbCapacity = MAX_RB);

    /**
     * 删除一个客户端
     */
    ~ClientInfo();

private:

    /**
     * 客户端开启线程：1缓存2分包3解包
     * @return 0:success -1:fail
     */
    int ProcessRecv();


private:

    /**
     * 接收数据缓冲线程
     * @param pClientInfo
     */
    static void ThreadDump(void *pClientInfo);

    /**
     * 分包线程
     * @param pClientInfo
     */
    static void ThreadGetPkg(void *pClientInfo);

    /**
     * 获取一包正文线程
     * @param pClientInfo
     */
    static void ThreadGetPkgContent(void *pClientInfo);

};

#ifdef __cplusplus
}
#endif

#endif //_CLIENTINFO_H
