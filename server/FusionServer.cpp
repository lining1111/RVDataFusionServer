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
#include <sstream>
#include <valarray>
#include "server/FusionServer.h"
#include "merge/merge.h"
#include "simpleini/SimpleIni.h"
#include "sqlite3.h"
#include "os/timeTask.hpp"
#include <functional>
#include "eoc_comm/configure_eoc_init.h"

using namespace os;

FusionServer::FusionServer(int port, bool isMerge, int cliNum) : TcpServer<ClientInfo>(port, "FusionServer-"+ to_string(port)) {
    this->isMerge = isMerge;
    unOrder.resize(cliNum);
    dataUnitFusionData.init(30, 100, cliNum, 10, this);//100ms一帧
    dataUnitTrafficFlowGather.init(30, 1000, cliNum, 10, this);//1000ms一帧
    dataUnitCrossTrafficJamAlarm.init(30, 1000, cliNum, 10, this);//1000ms一帧
    dataUnitIntersectionOverflowAlarm.init(30, 1000, cliNum, 10, this);//1000ms一帧
    dataUnitInWatchData_1_3_4.init(30, 1000, cliNum, 0, this);
    dataUnitInWatchData_2.init(30, 1000, cliNum, 0, this);
}

FusionServer::~FusionServer() {
    StopTimerTaskAll();
    Close();
//    StopLocalBusiness(this);
}

int FusionServer::getMatrixNoFromDb() {
    //打开数据库
    sqlite3 *db;
    string dbName;
#ifdef aarch64
    dbName = "/home/nvidianx/bin/CLParking.db";
#else
    dbName = "./db/CLParking.db";
#endif
    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite open fail db:" << dbName;
        sqlite3_close(db);
        return -1;
    }

    char *sql = "select UName from CL_ParkingArea";
    char **result, *errmsg;
    int nrow, ncolumn;
    string columnName;
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite err:" << errmsg;
        sqlite3_free(errmsg);
        return -1;
    }
    for (int m = 0; m < nrow; m++) {
        for (int n = 0; n < ncolumn; n++) {
            columnName = string(result[n]);
            if (columnName == "UName") {
                matrixNo = STR(result[ncolumn + n + m * nrow]);
                break;
            }
        }
    }
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}

int FusionServer::getPlatId() {
    this->plateId = string(g_eoc_intersection.PlatId);
    dataUnitFusionData.crossID = this->plateId;
//    dataUnitCarTrackGather.crossID = this->plateId;
    dataUnitTrafficFlowGather.crossID = this->plateId;
    dataUnitCrossTrafficJamAlarm.crossID = this->plateId;
    dataUnitIntersectionOverflowAlarm.crossID = this->plateId;
//    dataUnitLineupInfoGather.crossID = this->plateId;

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
    //获取matrixNo
    getMatrixNoFromDb();
    LOG(INFO) << "sn:" << matrixNo;
    //获取路口编号
//    getCrossIdFromDb();

//    //开启定时任务
//    std::shared_future<int> timerTask = std::async(std::launch::async, StartLocalBusiness, this);
//    timerTask.wait();

    return 0;
}

int FusionServer::Close() {
    TcpServer::Close();
    return 0;
}

int FusionServer::FindIndexInUnOrder(const string in) {
    int index = -1;
    //首先遍历是否已经存在
    int alreadyExistIndex = -1;
    for (int i = 0; i < unOrder.size(); i++) {
        auto &iter = unOrder.at(i);
        if (iter == in) {
            alreadyExistIndex = i;
            break;
        }
    }
    if (alreadyExistIndex >= 0) {
        index = alreadyExistIndex;
    } else {
        //不存在就新加
        for (int i = 0; i < unOrder.size(); i++) {
            auto &iter = unOrder.at(i);
            if (iter.empty()) {
                iter = in;
                index = i;
                break;
            }
        }
    }

    return index;
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


