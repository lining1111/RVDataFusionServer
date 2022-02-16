//
// Created by lining on 2/15/22.
// RV数据 接收服务端
//

#ifndef _FUSIONSERVER_H
#define _FUSIONSERVER_H

#include <cstdint>
#include <ctime>
#include <vector>
#include "server/ClientInfo.h"

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class FusionServer {
public:
    const uint16_t port = 5000;//暂定5000
    int maxListen = 5;//最大监听数

    const timeval checkStatusTimeval = {150, 0};//连续150s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};

    int sock = 0;//服务器socket
    //已连入的客户端列表
    vector<ClientInfo *> vector_client;


};

#ifdef __cplusplus
}
#endif

#endif //_FUSIONSERVER_H
