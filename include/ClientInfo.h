//
// Created by lining on 2/15/22.
//

#ifndef _CLIENTINFO_H
#define _CLIENTINFO_H

#include "RingBuffer.h"
#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;
class ClientInfo {
public:

    //接收buffer状态机 Start开始---GetHeadStart从接收的数据中寻找帧头开始(开始为特殊字符$)---GetHead找到帧头开始，获取全部帧头信息(一共9字节)---
    //GetBody获取全部帧头信息后，根据帧头信息的帧内容长度信息，获取全部帧内容---Start获取完全部帧内容后，回到开始状态
    typedef enum {
        Start = 0, GetHeadStart, GetHead, GetBody,
    } RecvStatus;
#define MAX_RB (1024*512)
public:
    //都是客户端的属性信息
    int msgid = 0;//帧号
    struct sockaddr_in clientAddr;
    int sock;//客户端socket
    int clientType = 0;
    unsigned char extraData[1024 * 8];//特性数据
    timeval receive_time;
    RecvStatus status = Start;
    RingBuffer *rb = nullptr;

public:
    /**
     * 初始化一个客户端对象
     * @param clientAddr 客户端的地址信息
     * @param client_sock 客户端socket
     * @param rbCapacity 客户端循环buffer的最大容量
     */
    ClientInfo(struct sockaddr_in clientAddr, int client_sock, long long int rbCapacity);

    /**
     * 删除一个客户端
     */
    ~ClientInfo();
};

#ifdef __cplusplus
}
#endif

#endif //_CLIENTINFO_H
