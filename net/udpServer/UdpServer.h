//
// Created by lining on 9/30/22.
//

#ifndef _UDPSERVER_H
#define _UDPSERVER_H


#include <stdint.h>
#include <netinet/in.h>
#include <future>

using namespace std;

class UdpServer {

public:
    int sock;
    pthread_mutex_t lockSend = PTHREAD_MUTEX_INITIALIZER;
    struct sockaddr_in server_addr, client_addr;

    string name;
    int port;
    bool isRun = false;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureGetRecv;
public:
    virtual void OnRecv(uint8_t *data, uint32_t len) = 0;

public:
    UdpServer(int port, string name);

    virtual ~UdpServer();

    int Open();

    int Close();

    void Run();

    int Send(uint8_t *data, uint32_t len, struct sockaddr_in addr);

private:
    int createSock();

    static int ThreadGetRecv(void *p);
};


#endif //_UDPSERVER_H
