//
// Created by lining on 2023/8/21.
//

#ifndef RVDATAFUSIONSERVER_MYTCPCLIENT_H
#define RVDATAFUSIONSERVER_MYTCPCLIENT_H

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/SocketConnector.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include <Poco/Exception.h>
#include "Poco/Observer.h"
#include "Poco/NObserver.h"
#include <glog/logging.h>
#include <iostream>

using Poco::Net::SocketReactor;
using Poco::Net::SocketConnector;
using Poco::Net::SocketAcceptor;
using Poco::Net::StreamSocket;
using Poco::Net::ServerSocket;
using Poco::Net::SocketAddress;
using Poco::Net::SocketNotification;
using Poco::Net::ReadableNotification;
using Poco::Net::WritableNotification;
using Poco::Net::TimeoutNotification;
using Poco::Net::ShutdownNotification;
using Poco::Observer;
using Poco::Net::NetException;
using Poco::Net::ConnectionRefusedException;
using Poco::Net::InvalidSocketException;
using Poco::TimeoutException;

#include "MyTcpHandler.hpp"

class MyTcpClient : public MyTcpHandler {
public:
    std::mutex *mtx = nullptr;
    string server_ip;
    int server_port;
    StreamSocket _s;
    std::string _peerAddress;
    char *recvBuf = nullptr;
    bool isNeedReconnect = true;
    std::thread _t;
public:
    MyTcpClient(string serverip, int serverport) :
            server_ip(serverip), server_port(serverport) {
        recvBuf = new char[1024 * 1024];
        if (mtx == nullptr) {
            mtx = new std::mutex();
        }
    }

    ~MyTcpClient() {
        LOG(WARNING) << _peerAddress << " disconnected ...";
        delete[]recvBuf;
        delete mtx;
    }

    int Open() {
        SocketAddress sa(server_ip, server_port);
        try {
            Poco::Timespan ts(1000 * 1000);
            _s.connect(sa, ts);
        } catch (ConnectionRefusedException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "connect refuse";
            return -1;
        }
        catch (TimeoutException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "connect time out";
            return -1;
        }
        catch (NetException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "net exception";
            return -1;
        }

        _peerAddress = _s.peerAddress().toString();
        LOG(WARNING) << "connection to " << _peerAddress << " ...";
        Poco::Timespan ts1(1000 * 100);
        _s.setSendTimeout(ts1);
        isNeedReconnect = false;

        return 0;
    }

    int Reconnect() {
        _s.close();
        SocketAddress sa(server_ip, server_port);
        try {
            Poco::Timespan ts(1000 * 1000);
            _s.connect(sa, ts);
        } catch (ConnectionRefusedException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "connect refuse";
            return -1;
        }
        catch (TimeoutException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "connect time out";
            return -1;
        }
        catch (NetException &) {
            LOG(ERROR) << server_ip << ":" << server_port << "net exception";
            return -1;
        }

        _peerAddress = _s.peerAddress().toString();
        LOG(WARNING) << "reconnection to " << _peerAddress << " ...";
        Poco::Timespan ts1(1000 * 100);
        _s.setSendTimeout(ts1);
        isNeedReconnect = false;

        return 0;
    }

    int Run() {
        _t = std::thread([this]() {
            LOG(INFO) << "_t start";
            while (this->_isRun) {
                usleep(10);
                if (!this->isNeedReconnect) {
                    bzero(recvBuf, 1024 * 1024);
                    int recvLen = (rb->GetWriteLen() < (1024 * 1024)) ? rb->GetWriteLen() : (1024 * 1024);
                    try {
                        int len = _s.receiveBytes(recvBuf, recvLen);
                        if (len < 0) {
                            LOG(ERROR) << server_ip << ":" << server_port << " receive len <0";
                            isNeedReconnect = true;
                        } else if (len > 0) {
                            rb->Write(recvBuf, len);
                        }
                    }
                    catch (Poco::Exception &exc) {
                        LOG(ERROR) << server_ip << ":" << server_port << " receive error:" << exc.code()
                                   << exc.displayText();
                        if (exc.code() != POCO_ETIMEDOUT) {
                            isNeedReconnect = true;
                        }
                    }
                }
            }
            LOG(INFO) << "_t end";
        });
        _t.detach();
        return 0;
    }

    int SendBase(common::Pkg pkg) {
        int ret = 0;
        //阻塞调用，加锁
        std::unique_lock<std::mutex> lock(*mtx);
        if (isNeedReconnect) {
            return -1;
        }

        uint8_t *buf_send = new uint8_t[1024 * 1024];
        uint32_t len_send = 0;
        len_send = 0;
        //pack
        common::Pack(pkg, buf_send, &len_send);
        try {
            auto len = _s.sendBytes(buf_send, len_send);
            VLOG(2) << server_ip << ":" << server_port << " send len:" << len << " len_send:" << len_send;
            if (len < 0) {
                LOG(ERROR) << server_ip << ":" << server_port << " send len < 0";
                isNeedReconnect = true;
                ret = -2;
            } else if (len != len_send) {
                LOG(ERROR) << server_ip << ":" << server_port << " send len !=len_send";
                isNeedReconnect = true;
                ret = -2;
            }
        }
        catch (Poco::Exception &exc) {
            LOG(ERROR) << server_ip << ":" << server_port << " send error:" << exc.code() << exc.displayText();
            if (exc.code() != POCO_ETIMEDOUT) {
                isNeedReconnect = true;
                ret = -2;
            } else {
                ret = -3;
            }
        }

        delete[] buf_send;
        return ret;
    }

    int Send(char *buf_send, int len_send) {
        int ret = 0;
        //阻塞调用，加锁
        std::unique_lock<std::mutex> lock(*mtx);
        if (isNeedReconnect) {
            return -1;
        }
        try {
            auto len = _s.sendBytes(buf_send, len_send);
            VLOG(2) << server_ip << ":" << server_port << " send len:" << len << " len_send:" << len_send;
            if (len < 0) {
                LOG(ERROR) << " send len < 0";
                isNeedReconnect = true;
                ret = -2;
            } else if (len != len_send) {
                LOG(ERROR) << " send len !=len_send";
                isNeedReconnect = true;
                ret = -2;
            }
        }
        catch (Poco::Exception &exc) {
            LOG(ERROR) << server_ip << ":" << server_port << " send error:" << exc.code() << exc.displayText();
            if (exc.code() != POCO_ETIMEDOUT) {
                isNeedReconnect = true;
                ret = -2;
            } else {
                ret = -3;
            }
        }

        delete[] buf_send;
        return ret;
    }
};

#endif //RVDATAFUSIONSERVER_MYTCPCLIENT_H
