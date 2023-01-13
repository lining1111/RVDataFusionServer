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
    dataUnitFusionData.init(30, 100, 100, cliNum);//100ms一帧
    dataUnitCarTrackGather.init(30, 66, 33, cliNum);//66ms一帧
    dataUnitTrafficFlowGather.init(30, 1000, 500, cliNum);//1000ms一帧
    dataUnitCrossTrafficJamAlarm.init(30, 1000, 500, cliNum);//1000ms一帧
    dataUnitLineupInfoGather.init(30, 1000, 500, cliNum);//1000ms一帧

}

FusionServer::~FusionServer() {
    StopTimerTaskAll();
    Close();
}

int FusionServer::getMatrixNoFromDb() {
    //打开数据库
    sqlite3 *db;
    string dbName;
#ifdef arm64
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

    isLocalBusinessRun = true;
    //开启定时任务
    std::shared_future<int> timerTask = std::async(std::launch::async, StartTimerTask, this);
    timerTask.wait();

    return 0;
}

int FusionServer::Close() {
    TcpServer::Close();
    isLocalBusinessRun = false;
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

int FusionServer::StartTimerTask(void *pServer) {

    if (pServer == nullptr) {
        return -1;
    }
    auto server = (FusionServer *) pServer;
    Notice("server :%d %s run", server->port, __FUNCTION__);

    server->addTimerTask("fusionServer timerFusionData", server->dataUnitFusionData.fs_ms,
                         std::bind(TaskFusionData, server, 10));

    server->addTimerTask("fusionServer timerTrafficFlowGather", server->dataUnitTrafficFlowGather.fs_ms,
                         std::bind(TaskTrafficFlowGather, server, 10));
    server->addTimerTask("fusionServer timerLineupInfoGather", server->dataUnitLineupInfoGather.fs_ms,
                         std::bind(TaskLineupInfoGather, server, 10));

    server->addTimerTask("fusionServer timerCrossTrafficJamAlarm", server->dataUnitCrossTrafficJamAlarm.fs_ms,
                         std::bind(TaskCrossTrafficJamAlarm, server, 10));

    server->addTimerTask("fusionServer timerCarTrackGather", server->dataUnitCarTrackGather.fs_ms,
                         std::bind(TaskCarTrackGather, server, 10));

    Notice("server :%d %s exit", server->port, __FUNCTION__);
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

void FusionServer::TaskFusionData(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitFusionData;
    if (server->isLocalBusinessRun) {
//        unique_lock<mutex> lck(dataUnit->mtx);
//        Info("%s", __FUNCTION__);
        //寻找同一帧,并处理
        DataUnitFusionData::MergeType mergeType;
        if (server->isMerge) {
            mergeType = DataUnitFusionData::Merge;
        } else {
            mergeType = DataUnitFusionData::NotMerge;
        }

        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitFusionData::TaskProcessOneFrame, mergeType);
    }
}

void FusionServer::TaskTrafficFlowGather(void *pServer, unsigned int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitTrafficFlowGather;
    if (server->isLocalBusinessRun) {
//        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitTrafficFlowGather::TaskProcessOneFrame);
    }
}

void FusionServer::TaskLineupInfoGather(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitLineupInfoGather;
    if (server->isLocalBusinessRun) {
//        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitLineupInfoGather::TaskProcessOneFrame);
    }
}

void FusionServer::TaskCrossTrafficJamAlarm(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitCrossTrafficJamAlarm;
    if (server->isLocalBusinessRun) {
//        unique_lock<mutex> lck(dataUnit->mtx);
//        Info("%s", __FUNCTION__);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame, false);
    }
}

void FusionServer::TaskCarTrackGather(void *pServer, int cache) {
    if (pServer == nullptr) {
        return;
    }
    auto server = (FusionServer *) pServer;
    auto dataUnit = &server->dataUnitCarTrackGather;
    if (server->isLocalBusinessRun) {
//        unique_lock<mutex> lck(dataUnit->mtx);
        //寻找同一帧,并处理
        dataUnit->FindOneFrame(cache, 1000 * 60, DataUnitCarTrackGather::TaskProcessOneFrame);
    }
}




