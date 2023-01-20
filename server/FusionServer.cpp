//
// Created by lining on 2/15/22.
//

#include <fcntl.h>
#include <iostream>
#include <log/Log.h>
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

using namespace z_log;
using namespace os;

FusionServer::FusionServer(int port, bool isMerge, int cliNum) : TcpServer<ClientInfo>(port, "FusionServer") {
    this->isMerge = isMerge;
    unOrder.resize(cliNum);
    dataUnitFusionData.init(30, 100, cliNum, 10);//100ms一帧
    dataUnitCarTrackGather.init(30, 33, cliNum, 10);//66ms一帧
    dataUnitTrafficFlowGather.init(30, 500, cliNum, 10);//1000ms一帧
    dataUnitCrossTrafficJamAlarm.init(30, 500, cliNum, 10);//1000ms一帧
    dataUnitLineupInfoGather.init(30, 500, cliNum, 10);//1000ms一帧
    StartLocalBusiness(this);
}

FusionServer::~FusionServer() {
    StopTimerTaskAll();
    Close();
    StopLocalBusiness(this);
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
        printf("sqlite open fail\n");
        sqlite3_close(db);
        return -1;
    }

    char *sql = "select UName from CL_ParkingArea";
    char **result, *errmsg;
    int nrow, ncolumn;
    string columnName;
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        Error("sqlite err:%s", errmsg);
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
    dataUnitCarTrackGather.crossID = this->plateId;
    dataUnitTrafficFlowGather.crossID = this->plateId;
    dataUnitCrossTrafficJamAlarm.crossID = this->plateId;
    dataUnitLineupInfoGather.crossID = this->plateId;

}

int FusionServer::Open() {

    int ret = TcpServer::Open();
    if (ret == 0) {
        Notice("server sock %d create success", sock);
    }

    return ret;
}

int FusionServer::Run() {
    TcpServer::Run();
    //获取matrixNo
    getMatrixNoFromDb();
    Notice("sn:%s", matrixNo.c_str());
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
        auto iter = unOrder.at(i);
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
            auto iter = unOrder.at(i);
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
    server->isLocalBusinessRun = true;
    Notice("server :%d %s start", server->port, __FUNCTION__);

    std::thread threadFusionData = std::thread([server]() {

        while (server->isLocalBusinessRun) {
            DataUnitFusionData *dataUnit = &server->dataUnitFusionData;
            DataUnitFusionData::MergeType mergeType;
            if (server->isMerge) {
                mergeType = DataUnitFusionData::Merge;
            } else {
                mergeType = DataUnitFusionData::NotMerge;
            }
            dataUnit->runTask(std::bind(dataUnit->FindOneFrame, dataUnit, (60 * 1000), mergeType, true));
        }
    });
    server->localBusinessThreadHandle.push_back(threadFusionData.native_handle());

    threadFusionData.detach();

    std::thread threadTrafficFlowGather = std::thread([server]() {
        while (server->isLocalBusinessRun) {
            DataUnitTrafficFlowGather *dataUnit = &server->dataUnitTrafficFlowGather;
            dataUnit->runTask(std::bind(dataUnit->FindOneFrame, dataUnit, (60 * 1000), true));
        }
    });
    server->localBusinessThreadHandle.push_back(threadTrafficFlowGather.native_handle());
    threadTrafficFlowGather.detach();

    std::thread threadLineupInfoGather = std::thread([server]() {
        while (server->isLocalBusinessRun) {
            DataUnitLineupInfoGather *dataUnit = &server->dataUnitLineupInfoGather;
            dataUnit->runTask(std::bind(dataUnit->FindOneFrame, dataUnit, (60 * 1000), true));
        }
    });
    server->localBusinessThreadHandle.push_back(threadLineupInfoGather.native_handle());
    threadLineupInfoGather.detach();

    std::thread threadCrossTrafficJamAlarm = std::thread([server]() {
        while (server->isLocalBusinessRun) {
            DataUnitCrossTrafficJamAlarm *dataUnit = &server->dataUnitCrossTrafficJamAlarm;
            dataUnit->runTask(std::bind(dataUnit->FindOneFrame, dataUnit, (60 * 1000), false));
        }
    });
    server->localBusinessThreadHandle.push_back(threadCrossTrafficJamAlarm.native_handle());
    threadCrossTrafficJamAlarm.detach();
    std::thread threadCarTrackGather = std::thread([server]() {
        while (server->isLocalBusinessRun) {
            DataUnitCarTrackGather *dataUnit = &server->dataUnitCarTrackGather;
            dataUnit->runTask(std::bind(dataUnit->FindOneFrame, dataUnit, (60 * 1000), true));
        }
    });
    server->localBusinessThreadHandle.push_back(threadCarTrackGather.native_handle());
    threadCarTrackGather.detach();

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

    server->isLocalBusinessRun = false;
//    for (auto &iter:server->localBusinessThreadHandle) {
//        pthread_cancel(iter);
//    }
    server->localBusinessThreadHandle.clear();

    Notice("server :%d %s stop", server->port, __FUNCTION__);
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


