//
// Created by lining on 10/1/22.
//

#ifndef _UDPSVRBROADCAST_H
#define _UDPSVRBROADCAST_H

#include <string>
#include <netinet/in.h>
#include <future>

using namespace std;
class UdpSvrBroadcast {
public:
    int sock;

    struct sockaddr_in server_addr;

    string name;
    string content;
    int interval;
    int port;
    bool isRun = false;
    bool isLocalThreadRun = false;
    std::shared_future<int> futureBroadcast;
public:
    UdpSvrBroadcast(int port, string name, string broadcastContent, int interval);
    virtual ~UdpSvrBroadcast();
    int Open();
    int Close();
    void Run();

private:
    int createSock();
    static int ThreadBroadcast(void *p);

};


#endif //_UDPSVRBROADCAST_H
