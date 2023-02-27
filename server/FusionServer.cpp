//
// Created by lining on 2/15/22.
//

#include <fcntl.h>
#include <iostream>
#include "glog/logging.h"
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include "server/FusionServer.h"
#include "simpleini/SimpleIni.h"
#include "sqlite3.h"
#include "os/timeTask.hpp"
#include <functional>

using namespace os;

FusionServer::FusionServer(int port) :
        TcpServer<ClientInfo>(port, "FusionServer-" + to_string(port)) {
}

FusionServer::~FusionServer() {
    StopTimerTaskAll();
    Close();
//    StopLocalBusiness(this);
}

int FusionServer::Open() {

    int ret = TcpServer::Open();
    if (ret == 0) {
        LOG(INFO) << "server sock create success:" << sock;
    }

    return ret;
}

int FusionServer::Run() {
    if (TcpServer::Run() != 0) {
        return -1;
    }
//    //开启定时任务
//    std::shared_future<int> timerTask = std::async(std::launch::async, StartLocalBusiness, this);
//    timerTask.wait();

    return 0;
}

int FusionServer::Close() {
    return TcpServer::Close();
}

int FusionServer::StartLocalBusiness(void *pServer) {

    if (pServer == nullptr) {
        return -1;
    }
    auto server = (FusionServer *) pServer;
    LOG(INFO) << "server:" << server->port << " StartLocalBusiness start";

//    std::thread([server]() {
//        while (server->isLocalBusinessRun) {
//            sleep(1);
//            Debug("server sock-%d test,port%d\n", server->sock, server->port);
//        }
//    }).detach();

    return 0;
}

int FusionServer::StopLocalBusiness(void *pServer) {
    if (pServer == nullptr) {
        return -1;
    }
    auto server = (FusionServer *) pServer;
//    for (auto &iter:server->localBusinessThreadHandle) {
//        pthread_cancel(iter);
//    }
    server->localBusinessThreadHandle.clear();

    LOG(INFO) << "server:" << server->port << " StopLocalBusiness stop";
    return 0;
}


void FusionServer::addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task) {
    Timer *timer = new Timer(name);
    timer->start(timeval_ms, task);
    timerTasks.insert(make_pair(name, timer));
}

void FusionServer::deleteTimerTask(string name) {
    auto timerTask = timerTasks.find(name);
    if (timerTask != timerTasks.end()) {
        delete timerTask->second;
        timerTasks.erase(timerTask++);
    }
}

void FusionServer::StopTimerTaskAll() {
    auto iter = timerTasks.begin();
    while (iter != timerTasks.end()) {
        delete iter->second;
        iter = timerTasks.erase(iter);
    }
}


