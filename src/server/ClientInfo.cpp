//
// Created by lining on 2/15/22.
//

#include <cstring>
#include <sys/time.h>
#include "server/ClientInfo.h"


ClientInfo::ClientInfo(struct sockaddr_in clientAddr, int client_sock, long long int rbCapacity) {
    this->msgid = 0;
    this->clientAddr = clientAddr;
    this->sock = client_sock;
    this->clientType = 0;
    bzero(this->extraData, sizeof(this->extraData) / sizeof(this->extraData[0]));
    gettimeofday(&this->receive_time, nullptr);
    this->rb = RingBuffer_New(rbCapacity);
    this->status = Start;
}

ClientInfo::~ClientInfo() {
    RingBuffer_Delete(this->rb);
}
