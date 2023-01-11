//
// Created by lining on 9/30/22.
//

#ifndef _UDPCLIENT_H
#define _UDPCLIENT_H

#include <string>
#include <netinet/in.h>
#include <future>

using namespace std;

class UdpClient {
public:
    int sock;
    pthread_mutex_t lockSend = PTHREAD_MUTEX_INITIALIZER;
    struct sockaddr_in server_addr;

    string name;
    string serverIp;
    int serverPort;
    bool isRun = false;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureGetRecv;
public:
    virtual void OnRecv(uint8_t *data, uint32_t len) = 0;

public:
    UdpClient(string ip, int port, string name);

    virtual ~UdpClient();

    int Open();

    int Close();

    void Run();

    int Send(uint8_t *data, uint32_t len);

private:
    int createSock();

    static int ThreadGetRecv(void *p);
};


#endif //_UDPCLIENT_H
