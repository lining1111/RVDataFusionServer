//
// Created by lining on 2023/8/21.
//

#ifndef RVDATAFUSIONSERVER_TCPSERVER_H
#define RVDATAFUSIONSERVER_TCPSERVER_H

#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/SocketConnector.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/SocketAddress.h"
#include "Poco/Net/NetException.h"
#include "Poco/Observer.h"
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

#include "MyTcpHandler.hpp"

class MyTcpServerHandler : public MyTcpHandler {
public:
    StreamSocket _socket;
    SocketReactor &_reactor;
    char *recvBuf = nullptr;
public:
    MyTcpServerHandler(StreamSocket &socket, SocketReactor &reactor) :
            _socket(socket),
            _reactor(reactor) {
        _peerAddress = socket.peerAddress().toString();
        LOG(WARNING) << "connection from " << _peerAddress << " ...";
        std::unique_lock<std::mutex> lock(conns_mutex);
        conns.push_back(this);
        _reactor.addEventHandler(_socket, Observer<MyTcpServerHandler, ReadableNotification>(*this,
                                                                                             &MyTcpServerHandler::onReadable));
        _reactor.addEventHandler(_socket, NObserver<MyTcpServerHandler, ShutdownNotification>(*this,
                                                                                              &MyTcpServerHandler::onSocketShutdown));
        recvBuf = new char[1024 * 1024];
    }

    ~MyTcpServerHandler() {
        LOG(WARNING) << _peerAddress << " disconnected ...";
        _reactor.removeEventHandler(_socket, Observer<MyTcpServerHandler, ReadableNotification>(*this,
                                                                                                &MyTcpServerHandler::onReadable));
        _reactor.removeEventHandler(_socket, NObserver<MyTcpServerHandler, ShutdownNotification>(*this,
                                                                                                 &MyTcpServerHandler::onSocketShutdown));
        delete[]recvBuf;
        std::unique_lock<std::mutex> lock(conns_mutex);
        for (int i = 0; i < conns.size(); i++) {
            auto conn = (MyTcpServerHandler *) conns.at(i);
            if (conn->_peerAddress == _peerAddress) {
                conns.erase(conns.begin() + i);
                LOG(WARNING) << "从数组踢出客户端:" << conn->_peerAddress;
            }
        }
    }

    void onReadable(ReadableNotification *pNf) {
        pNf->release();
        bzero(recvBuf, 1024 * 1024);
        int recvLen = (rb->GetWriteLen() < (1024 * 1024)) ? rb->GetWriteLen() : (1024 * 1024);
        try {
            int len = _socket.receiveBytes(recvBuf, recvLen);
            if (len <= 0) {
                delete this;
            } else {
                rb->Write(recvBuf, len);
            }
        }
        catch (Poco::Exception &exc) {
            pNf->release();
            delete this;
        }
    }

    void onSocketShutdown(const AutoPtr<ShutdownNotification> &pNf) {
        pNf->release();
        delete this;
    }
};

class MyTcpServer {
public:
    int _port;
    Poco::Net::ServerSocket _s;
    Poco::Net::SocketReactor _reactor;
    Poco::Thread _t;
    Poco::Net::SocketAcceptor<MyTcpServerHandler> *_acceptor = nullptr;
    bool isListen = false;
public:
    MyTcpServer(int port) : _port(port) {

    }

    ~MyTcpServer() {
        delete _acceptor;
    }

    int Open() {
        try {
            _s.bind(Poco::Net::SocketAddress(_port));
            _s.listen();
        } catch (Poco::Exception &exc) {
            LOG(ERROR) << exc.what();
            isListen = false;
            return -1;
        }
        _acceptor = new Poco::Net::SocketAcceptor<MyTcpServerHandler>(_s, _reactor);
        isListen = true;
        return 0;
    }

    int ReOpen() {
        try {
            _s.close();
            _s.bind(Poco::Net::SocketAddress(_port));
            _s.listen();
        } catch (Poco::Exception &exc) {
            LOG(ERROR) << exc.what();
            isListen = false;
            return -1;
        }
        if (_acceptor != nullptr) {
            delete _acceptor;
        }
        _acceptor = new Poco::Net::SocketAcceptor<MyTcpServerHandler>(_s, _reactor);
        isListen = true;
        return 0;
    }


    int Run() {
        //Starting TCP Server
        _t.start(_reactor);
        LOG(WARNING) << _port << "-Server Started";
        LOG(WARNING) << _port << "-Ready To Accept the connections";
        return 0;
    }

};


#endif //RVDATAFUSIONSERVER_TCPSERVER_H
