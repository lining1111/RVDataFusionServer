//
// Created by lining on 2/18/22.
//


#include <iostream>
#include <string>
#include <thread>


#include "myTcp/MyTcpServer.hpp"

int main(int argc, char **argv) {       //Setting Server IP address
    MyTcpServer *server = new MyTcpServer(1234);
    if (server->Open() == 0) {
        server->Run();
    }

    MyTcpServer *server1 = new MyTcpServer(12345);
    if (server1->Open() == 0) {
        server1->Run();
    }


    while (1) {
        sleep(5);
        printf("conns size:%d\n",conns.size());
        for (auto conn: conns) {
            printf("conn :%s\n", ((MyTcpServerHandler *) conn)->_peerAddress.c_str());
        }
    }
}