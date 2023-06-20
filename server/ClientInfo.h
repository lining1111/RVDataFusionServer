//
// Created by lining on 2/15/22.
//

#ifndef _CLIENTINFO_H
#define _CLIENTINFO_H

#include "ringbuffer/RingBuffer.h"
#include "common/common.h"
#include <netinet/in.h>
#include <queue>
#include <future>
#include <stdbool.h>
#include <atomic>
#include "Queue.hpp"
#include "net/tcpServer/TcpServerClient.hpp"

using namespace std;
using namespace common;

class ClientInfo : public TcpServerClient {
public:

    //接收buffer状态机 Start开始---从接收的数据中寻找帧头开始(开始为特殊字符$)GetHeadStart---找到帧头开始，获取全部帧头信息(一共9字节)GetHead---
    //获取全部帧头信息后，根据帧头信息的帧内容长度信息，获取全部帧内容GetBody---获取完身体后，获取CRC GETCRC---获取完全部帧内容后，回到开始状态Start
    typedef enum {
        Start = 0, GetHeadStart, GetHead, GetBody, GetCRC,
    } RecvStatus;//表示状态机已经达到的状态
#define MAX_RB (1024*1024*64)
public:
    //都是客户端的属性信息
    void *super = nullptr;
    int type = 0;
    vector<uint8_t> extraData;//特性数据
    bool isReceive_timeSet = false;
    atomic_int direction;//方向,在解包的时候更新
private:
    RecvStatus status = Start;
    //用于缓存解包
    PkgHead pkgHead;//分包头
    int bodyLen = 0;//获取分包头后，得到的包长度
    uint8_t *pkgBuffer = nullptr;
    int index = 0;
public:
    //供给服务端使用的变量
    atomic_bool needRelease;//客户端处理线程在发现sock异常时，向上抛出释放队列内容信号
    bool isThreadRun = false;
    //客户端处理线程
    //GetPkg 生产者 GetPkgContent消费者 通过queue加锁的方式完成传递
    Queue<Pkg> queuePkg;
    //从包队列中依据方法名获取正文结构体，有多少方法名就有多少队列
    std::shared_future<int> futureGetPkg;
    std::shared_future<int> futureGetPkgContent;

public:
    /**
     * 初始化一个客户端对象
     * @param clientAddr 客户端的地址信息
     * @param client_sock 客户端socket
     * @param super
     * @
     * @param rbCapacity 客户端循环buffer的最大容量
     */
    ClientInfo(int client_sock, struct sockaddr_in clientAddr, string name, void *super,
               long long int rbCapacity = MAX_RB, timeval *readTimeout = nullptr);

    /**
     * 删除一个客户端
     */
    ~ClientInfo();

    int Close();

public:

    /**
     * 客户端开启线程：1缓存2分包3解包
     * @return 0:success -1:fail
     */
    int Run();


private:

    /**
     * 分包线程
     * @param pClientInfo
     */
    static int ThreadGetPkg(void *pClientInfo);

    /**
     * 获取一包正文线程
     * @param pClientInfo
     */
    static int ThreadGetPkgContent(void *pClientInfo);

};


#endif //_CLIENTINFO_H
