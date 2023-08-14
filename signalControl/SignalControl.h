//
// Created by lining on 12/26/22.
//

#ifndef _SIGNALCONTROL_H
#define _SIGNALCONTROL_H


#include <string>
#include <netinet/in.h>
#include "signalControlCom.h"

using namespace ComFrame_GBT20999_2017;

class SignalControl {
public:
    std::string serverIP;
    uint32_t serverPort;
    struct sockaddr_in remote_addr;
    struct sockaddr_in local;
    uint32_t localPort;
    uint32_t sn;
    bool isOpen = false;
    int sock = 0;
public:
    SignalControl(std::string _serverIP, uint32_t _serverPort, uint32_t _localPort);

    ~SignalControl();

    int Open();

    int Close();

    int SetRecvTimeout(uint8_t timeout);

    int Communicate(FrameAll sendFrame, FrameAll &recvFrame);
};


#endif //_SIGNALCONTROL_H
