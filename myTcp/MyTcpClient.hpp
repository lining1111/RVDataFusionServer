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

class MyTcpClient :public MyTcpHandler{
public:
    enum {
        BUFFER_SIZE = 1024 * 1024 * 64
    };
    std::mutex *mtx= nullptr;
    string server_ip;
    int server_port;
    StreamSocket _socket;
    std::string _peerAddress;
    char *recvBuf = nullptr;
    bool isNeedReconnect = true;
    std::thread _t;
public:
    MyTcpClient(string serverip, int serverport) :
            server_ip(serverip), server_port(serverport) {
        recvBuf = new char[1024 * 1024];
        if (mtx == nullptr){
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
            _socket.connect(sa, ts);
        } catch (ConnectionRefusedException&)
        {
            std::cout<<server_ip<<":"<<server_port<<"connect refuse"<<std::endl;
            return -1;
        }
        catch (NetException&)
        {
            std::cout<<server_ip<<":"<<server_port<<"net exception"<<std::endl;
            return -1;
        }
        catch (TimeoutException&)
        {
            std::cout<<server_ip<<":"<<server_port<<"connect time out"<<std::endl;
            return -1;
        }

        _peerAddress = _socket.peerAddress().toString();
        LOG(WARNING) << "connection to " << _peerAddress << " ...";
        Poco::Timespan ts1(1000 * 100);
        _socket.setSendTimeout(ts1);
        isNeedReconnect = false;

        return 0;
    }

    int Reconnect() {
        _socket.close();
        SocketAddress sa(server_ip, server_port);
        try {
            Poco::Timespan ts(1000 * 1000);
            _socket.connect(sa, ts);
        } catch (...) {
            return -1;
        }
        _peerAddress = _socket.peerAddress().toString();
        LOG(WARNING) << "reconnection to " << _peerAddress << " ...";
        Poco::Timespan ts1(1000 * 100);
        _socket.setSendTimeout(ts1);
        isNeedReconnect = false;

        return 0;
    }

    int Run() {
        _t = std::thread([this]() {
            LOG(INFO)<<"_t start";
            while(this->_isRun){
                usleep(10);
                if(!this->isNeedReconnect){
                    bzero(recvBuf, 1024 * 1024);
                    int recvLen = (rb->GetWriteLen() < (1024 * 1024)) ? rb->GetWriteLen() : (1024 * 1024);
                    try {
                        int len = _socket.receiveBytes(recvBuf, recvLen);
                        if (len <= 0) {
                            isNeedReconnect = true;
                        } else {
                            rb->Write(recvBuf, len);
                        }
                    }
                    catch (Poco::Exception &exc) {
                        isNeedReconnect = true;
                    }
                }
            }
            LOG(INFO)<<"_t end";
        });
        _t.detach();
        cout << "Client Started" << endl;
        return 0;
    }

    int SendBase(common::Pkg pkg) {
        int ret = 0;
        //阻塞调用，加锁
        std::unique_lock<std::mutex> lock(*mtx);
        if (isNeedReconnect) {
            return -3;
        }

        uint8_t *buf_send = new uint8_t[1024 * 1024];
        uint32_t len_send = 0;
        len_send = 0;
        //pack
        common::Pack(pkg, buf_send, &len_send);
        try {
            auto len = _socket.sendBytes(buf_send, len_send);

            if (len < 0) {
                isNeedReconnect = true;
                ret = -2;
            } else if (len != len_send) {
                isNeedReconnect = true;
                ret = -2;
            }
        } catch (...) {
            isNeedReconnect = true;
            ret = -2;
        }
        delete[] buf_send;
        return ret;
    }

    int Send(char *buf_send, int len_send) {
        int ret = 0;
        //阻塞调用，加锁
        std::unique_lock<std::mutex> lock(*mtx);
        if (isNeedReconnect) {
            return -3;
        }
        len_send = 0;
        try {
            auto len = _socket.sendBytes(buf_send, len_send);

            if (len < 0) {
                isNeedReconnect = true;
                ret = -2;
            } else if (len != len_send) {
                isNeedReconnect = true;
                ret = -2;
            }
        } catch (...) {
            isNeedReconnect = true;
            ret = -2;
        }
        delete[] buf_send;
        return ret;
    }
};

#endif //RVDATAFUSIONSERVER_MYTCPCLIENT_H
