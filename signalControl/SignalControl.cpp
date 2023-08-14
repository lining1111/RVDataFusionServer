//
// Created by lining on 12/26/22.
//

#include "SignalControl.h"
#include <glog/logging.h>
#include <bits/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

SignalControl::SignalControl(std::string _serverIP, uint32_t _serverPort, uint32_t _localPort) {
    serverIP = _serverIP;
    serverPort = _serverPort;
    localPort = _localPort;
    isOpen = false;
}

SignalControl::~SignalControl() {
    if (isOpen) {
        Close();
    }
}

int SignalControl::Open() {
    //开启udp客户端
    LOG(INFO) << "SignalControl " << "udp:" << serverIP << ":" << serverPort;
    int socketFd;
    // Create UDP socket:
    socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (socketFd < 0) {
        LOG(ERROR) << "SignalControl " << "Error while creating socket";
        return -1;
    }
    LOG(INFO) << "SignalControl " << "Socket created successful";

    // Set port and IP:
    auto addrLen = sizeof(struct sockaddr_in);
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(serverPort);
    remote_addr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    memset(&local, 0, sizeof(struct sockaddr_in));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(localPort);//UDP 广播包 本地端口
    socklen_t local_len = sizeof(struct sockaddr);


    if (bind(socketFd, (struct sockaddr *) &local, sizeof(local)))//绑定端口
    {
        LOG(ERROR) << "SignalControl " << "client bind port failed";
        close(socketFd);//关闭socket
        return -1;
    }

    timeval timeout;
//    timeout.tv_sec = 10;
    timeout.tv_usec = 100 * 1000;
    //设置发送最大时间80ms
//    setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    LOG(INFO) << "SignalControl " << "udp client sock:" << socketFd;

    sock = socketFd;
    isOpen = true;

    return 0;
}

int SignalControl::Close() {
    if (sock > 0) {
        close(sock);
        sock = 0;
    }
    isOpen = false;
    return 0;
}

int SignalControl::SetRecvTimeout(uint8_t timeout) {
    timeval tv;
    tv.tv_sec = timeout;
//    timeout.tv_usec = 100 * 1000;
    //设置发送最大时间80ms
//    setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(struct timeval));
    return 0;
}

int SignalControl::Communicate(FrameAll sendFrame, FrameAll &recvFrame) {
    if (!isOpen) {
        LOG(ERROR) << "SignalControl not open";
        return -1;
    }

    vector<uint8_t> sendBytes;
    sendFrame.setToBytesWithTF(sendBytes);
//    printf("sendBytes:");
//    for (auto iter: sendBytes) {
//        printf("%02x ", iter);
//    }
//    printf("\n");

    auto addrLen = sizeof(struct sockaddr_in);
    int lenS = sendto(sock, sendBytes.data(), sendBytes.size(), 0, (struct sockaddr *) &remote_addr,
                      addrLen);
    if (lenS != sendBytes.size()) {
        LOG(ERROR) << "SignalControl send fail err:" << errno;
        return -1;
    } else {
        LOG(INFO) << "SignalControl send ok";
        uint8_t bufR[1024];
        memset(bufR, 0, 1024);
        int lenR = recvfrom(sock, bufR, 1024, 0, (struct sockaddr *) &remote_addr, (socklen_t *) &addrLen);
        if (lenR > 0) {
            LOG(INFO) << "SignalControl recv ok";
            vector<uint8_t> recvBytes;
            for (int i = 0; i < lenR; i++) {
//                printf("%02x ", bufR[i]);
                recvBytes.push_back(bufR[i]);
            }
//            printf("\n");

            if (recvFrame.getFromBytesWithTF(recvBytes) == 0) {
//                vector<uint8_t> getPlainBytes;
//                getPlainBytes.clear();
//                recvFrame.setToBytes(getPlainBytes);
//
//                printf("getPlainBytes:");
//                for (auto iter: getPlainBytes) {
//                    printf("%02x ", iter);
//                }
//                printf("\n");
            }
        } else {
            LOG(ERROR) << "SignalControl recv fail:" << errno;
            return -1;
        }
    }

    return 0;
}
