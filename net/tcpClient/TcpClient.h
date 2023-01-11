//
// Created by lining on 9/30/22.
//

#ifndef _TCPCLIENT_H
#define _TCPCLIENT_H

#include <string>
#include "ringbuffer/RingBuffer.h"
#include <future>

using namespace std;

class TcpClient {
public:
    enum State {

    };
public:
    string serverIp;
    int serverPort;
    string name;
    int sock = -1;
    bool isAsynReceive = true;
    pthread_mutex_t lockSend = PTHREAD_MUTEX_INITIALIZER;
protected:
    bool isConnect = false;
    bool isSetRecvTimeout = false;
    struct timeval recvTimeout;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureDump;
    timeval receive_time;
#define RecvSize (1024*1024*10)
    RingBuffer *rb = nullptr;

public:
    bool GetConnectState();

public:
    TcpClient(string ip, int port, string name, timeval *readTimeout = nullptr);

    virtual ~TcpClient();

    int Open();

    int Close();

    int Run();

    pthread_mutex_t lockSock = PTHREAD_MUTEX_INITIALIZER;

    int Send(uint8_t *data, uint32_t len);

private:
    int createSock();

    static int ThreadDump(void *local);
};


#endif //_TCPCLIENT_H
