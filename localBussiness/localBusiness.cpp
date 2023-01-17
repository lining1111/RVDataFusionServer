//
// Created by lining on 11/22/22.
//

#include <functional>
#include "localBusiness.h"
#include <arpa/inet.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "log/Log.h"

using namespace z_log;

static string savePath = "/mnt/mnt_hd/save/";
static int saveCountMax = 0;

LocalBusiness::LocalBusiness() {

}

LocalBusiness::~LocalBusiness() {
    StopTimerTaskAll();
    isRun = false;
    for (auto iter = clientList.begin(); iter != clientList.end();) {
        delete iter->second;
        iter = clientList.erase(iter);
    }
    for (auto iter = serverList.begin(); iter != serverList.end();) {
        delete iter->second;
        iter = serverList.erase(iter);
    }
}

void LocalBusiness::AddServer(string name, int port, bool isMerge) {
    FusionServer *server = new FusionServer(port, isMerge, 4);
    server->port = port;
    serverList.insert(make_pair(name, server));
}

void LocalBusiness::AddClient(string name, string cloudIp, int cloudPort, void *super) {
    FusionClient *client = new FusionClient(cloudIp, cloudPort, super);//端口号和ip依实际情况而变
    clientList.insert(make_pair(name, client));
}


void LocalBusiness::Run() {
    isRun = true;
    for (auto &iter:serverList) {
        iter.second->Open();
        iter.second->Run();
    }
    for (auto &iter:clientList) {
        iter.second->Open();
        iter.second->Run();

    }
    if (0) {
        string dirName1 = savePath;
        int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (!isCreate1)
            Info("create path:%s\n", dirName1.c_str());
        else
            Info("create path failed! error code : %d \n", isCreate1);
    }

    StartTimerTask();
}

void LocalBusiness::addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task) {
    Timer *timer = new Timer(name);
    timer->start(timeval_ms, task);
    timerTasks.insert(make_pair(name, timer));
}

void LocalBusiness::deleteTimerTask(string name) {
    auto timerTask = timerTasks.find(name);
    if (timerTask != timerTasks.end()) {
        delete timerTask->second;
        timerTasks.erase(timerTask++);
    }
}

void LocalBusiness::StartTimerTask() {

    addTimerTask("localBusiness timerKeep", 1000, std::bind(Task_Keep, this));

    std::thread([this]() {
        while (this->isRun) {
            Task_FusionData(this);
        }
    }).detach();

    std::thread([this]() {
        while (this->isRun) {
            Task_TrafficFlowGather(this);
        }
    }).detach();


    std::thread([this]() {
        while (this->isRun) {
            Task_LineupInfoGather(this);
        }
    }).detach();

    std::thread([this]() {
        while (this->isRun) {
            Task_CrossTrafficJamAlarm(this);
        }
    }).detach();

    std::thread([this]() {
        while (this->isRun) {
            Task_CarTrackGather(this);
        }
    }).detach();


    //开启伪造数据线程
//    addTimerTask("localBusiness timerCreateCrossTrafficJamAlarm",10*1000,std::bind(Task_CreateCrossTrafficJamAlarm,this));
//    addTimerTask("localBusiness timerCreateLineupInfoGather",1000,std::bind(Task_CreateLineupInfoGather,this));
//    addTimerTask("localBusiness timerCreateTrafficFlowGather",1000,std::bind(Task_CreateTrafficFlowGather,this));
}


void LocalBusiness::StopTimerTaskAll() {
    auto iter = timerTasks.begin();
    while (iter != timerTasks.end()) {
        delete iter->second;
        iter = timerTasks.erase(iter);
    }
}


void LocalBusiness::Task_Keep(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (LocalBusiness *) p;

    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        for (auto &iter:local->serverList) {
            if (!iter.second->isRun) {
                iter.second->Close();
                if (iter.second->Open() == 0) {
                    Notice("服务端 %s 重启", iter.first.c_str());
                    iter.second->Run();
                }
            }
        }

        for (auto &iter:local->clientList) {
            if (!iter.second->isRun) {
                iter.second->Close();
                if (iter.second->Open() == 0) {
                    Notice("客户端 %s 重启", iter.first.c_str());
                    iter.second->Run();
                }
            }
        }
    }
}

void LocalBusiness::Task_CarTrackGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    string msgType = "CarTrackGather";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitCarTrackGather;

        unique_lock<mutex> lck(dataUnit->mtx_o);

        dataUnit->cv_o_task.wait(lck);


        CarTrackGather data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgCarTrackGatherWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            //存发送
            if (0) {
                string dirName1 = savePath + msgType;
                int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                if (!isCreate1)
                    Info("create path:%s\n", dirName1.c_str());
                else
                    Info("create path failed! error code : %d \n", isCreate1);
                if (saveCountMax > 0) {
                    if (dataUnit->saveCount < saveCountMax) {
                        dataUnit->saveCount++;
                        string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                        ofstream file;
                        file.open(fileName);
                        if (file.is_open()) {
                            file.write(pkg.body.data(), pkg.body.size());
                            file.flush();
                            file.close();
                        }
                    }
                } else {
                    dataUnit->saveCount++;
                    string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                    ofstream file;
                    file.open(fileName);
                    if (file.is_open()) {
                        file.write(pkg.body.data(), pkg.body.size());
                        file.flush();
                        file.close();
                    }
                }

            }

            if (local->clientList.empty()) {
                Error("client list empty");
                continue;
            }

            for (auto &iter1:local->clientList) {
                auto cli = iter1.second;
                if (cli->isRun) {
                    Info("server %s 发送到上层 %s,消息%s,matrixNo:%d",
                         iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                    if (cli->SendBase(pkg) == -1) {
                        Warn("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                    } else {
                        Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        cli->fsDynamicCarTrackGather.Update();
                    }
                } else {
                    Warn(":未连接上层%s", cli->server_ip.c_str());
                }
            }
            dataUnit->UpdateFSO();
        }
    }
}

void LocalBusiness::Task_CrossTrafficJamAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    string msgType = "CrossTrafficJamAlarm";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitCrossTrafficJamAlarm;
        unique_lock<mutex> lck(dataUnit->mtx_o);

        dataUnit->cv_o_task.wait(lck);

        CrossTrafficJamAlarm data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgCrossTrafficJamAlarmWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;

            //存发送
            if (0) {
                string dirName1 = savePath + msgType;
                int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                if (!isCreate1)
                    Info("create path:%s\n", dirName1.c_str());
                else
                    Info("create path failed! error code : %d \n", isCreate1);

                if (saveCountMax > 0) {
                    if (dataUnit->saveCount < saveCountMax) {
                        dataUnit->saveCount++;
                        string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                        ofstream file;
                        file.open(fileName);
                        if (file.is_open()) {
                            file.write(pkg.body.data(), pkg.body.size());
                            file.flush();
                            file.close();
                        }
                    }
                } else {
                    dataUnit->saveCount++;
                    string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                    ofstream file;
                    file.open(fileName);
                    if (file.is_open()) {
                        file.write(pkg.body.data(), pkg.body.size());
                        file.flush();
                        file.close();
                    }
                }
            }

            if (local->clientList.empty()) {
                Error("client list empty");
                continue;
            }

            for (auto &iter1:local->clientList) {
                auto cli = iter1.second;
                if (cli->isRun) {
                    Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                         iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);

                    if (cli->SendBase(pkg) == -1) {
                        Warn("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                    } else {
                        Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        cli->fsDynamicCrossTrafficJamAlarm.Update();
                    }
                } else {
                    Warn(":未连接上层%s", cli->server_ip.c_str());
                }
            }
            dataUnit->UpdateFSO();
        }
    }
}

void LocalBusiness::Task_LineupInfoGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    string msgType = "LineupInfoGather";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitLineupInfoGather;
        unique_lock<mutex> lck(dataUnit->mtx_o);

        dataUnit->cv_o_task.wait(lck);

        LineupInfoGather data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgLineupInfoGatherWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            //存发送
            if (0) {
                string dirName1 = savePath + msgType;
                int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                if (!isCreate1)
                    Info("create path:%s\n", dirName1.c_str());
                else
                    Info("create path failed! error code : %d \n", isCreate1);

                if (saveCountMax > 0) {
                    if (dataUnit->saveCount < saveCountMax) {
                        dataUnit->saveCount++;
                        string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                        ofstream file;
                        file.open(fileName);
                        if (file.is_open()) {
                            file.write(pkg.body.data(), pkg.body.size());
                            file.flush();
                            file.close();
                        }
                    }
                } else {
                    dataUnit->saveCount++;
                    string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                    ofstream file;
                    file.open(fileName);
                    if (file.is_open()) {
                        file.write(pkg.body.data(), pkg.body.size());
                        file.flush();
                        file.close();
                    }
                }
            }

            if (local->clientList.empty()) {
                Error("client list empty");
                continue;
            }

            for (auto &iter1:local->clientList) {
                auto cli = iter1.second;
                if (cli->isRun) {
                    Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                         iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                    if (cli->SendBase(pkg) == -1) {
                        Warn("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                    } else {
                        Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        cli->fsDynamicLineupInfoGather.Update();
                    }
                } else {
                    Warn("未连接上层%s", cli->server_ip.c_str());
                }
            }
            dataUnit->UpdateFSO();
        }
    }
}


void LocalBusiness::Task_TrafficFlowGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    string msgType = "TrafficFlowGather";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitTrafficFlowGather;
        unique_lock<mutex> lck(dataUnit->mtx_o);

        dataUnit->cv_o_task.wait(lck);

        TrafficFlowGather data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgTrafficFlowGatherWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            //存发送
            if (0) {
                string dirName1 = savePath + msgType;
                int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                if (!isCreate1)
                    Info("create path:%s\n", dirName1.c_str());
                else
                    Info("create path failed! error code : %d \n", isCreate1);

                if (saveCountMax > 0) {
                    if (dataUnit->saveCount < saveCountMax) {
                        dataUnit->saveCount++;
                        string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                        ofstream file;
                        file.open(fileName);
                        if (file.is_open()) {
                            file.write(pkg.body.data(), pkg.body.size());
                            file.flush();
                            file.close();
                        }
                    }
                } else {
                    dataUnit->saveCount++;
                    string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timestamp) + ".txt";
                    ofstream file;
                    file.open(fileName);
                    if (file.is_open()) {
                        file.write(pkg.body.data(), pkg.body.size());
                        file.flush();
                        file.close();
                    }
                }
            }

            if (local->clientList.empty()) {
                Error("client list empty");
                continue;
            }

            for (auto &iter1:local->clientList) {
                auto cli = iter1.second;
                if (cli->isRun) {
                    Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                         iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                    if (cli->SendBase(pkg) == -1) {
                        Warn("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                    } else {
                        Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        cli->fsDynamicTrafficFlowGather.Update();
                    }
                } else {
                    Warn("未连接上层%s", cli->server_ip.c_str());
                }
            }
            dataUnit->UpdateFSO();
        }
    }
}

void LocalBusiness::Task_FusionData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    string msgType = "FusionData";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitFusionData;
        unique_lock<mutex> lck(dataUnit->mtx_o);

        dataUnit->cv_o_task.wait(lck);

        FusionData data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgFusionDataWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            //存发送
            if (0) {
                string dirName1 = savePath + msgType;
                int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
                if (!isCreate1)
                    Info("create path:%s\n", dirName1.c_str());
                else
                    Info("create path failed! error code : %d \n", isCreate1);

                if (saveCountMax > 0) {
                    if (dataUnit->saveCount < saveCountMax) {
                        dataUnit->saveCount++;
                        string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timstamp) + ".txt";
                        ofstream file;
                        file.open(fileName);
                        if (file.is_open()) {
                            file.write(pkg.body.data(), pkg.body.size());
                            file.flush();
                            file.close();
                        }
                    }
                } else {
                    dataUnit->saveCount++;
                    string fileName = savePath + msgType + "/" + to_string((uint64_t) data.timstamp) + ".txt";
                    ofstream file;
                    file.open(fileName);
                    if (file.is_open()) {
                        file.write(pkg.body.data(), pkg.body.size());
                        file.flush();
                        file.close();
                    }
                }
            }

            if (local->clientList.empty()) {
                Error("client list empty");
                continue;
            }

            for (auto &iter1:local->clientList) {
                auto cli = iter1.second;
                if (cli->isRun) {
                    Info("server %s 发送到上层%s,消息%s,matrixNo:%d,sn:%d,length:%d",
                         iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO, pkg.head.sn,
                         pkg.head.len);
                    if (cli->SendBase(pkg) == -1) {
                        Warn("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                        cli->packetLossFusionData->Fail();
                    } else {
                        Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        cli->packetLossFusionData->Success();
                        cli->fsDynamicFusionData.Update();
                    }
                } else {
                    Warn("未连接上层%s", cli->server_ip.c_str());
                    cli->packetLossFusionData->Fail();
                }
            }
            dataUnit->UpdateFSO();
        }
    }
}

void LocalBusiness::Task_CreateCrossTrafficJamAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto server = local->serverList.find("server1")->second;
    auto dataUnit = &server->dataUnitCrossTrafficJamAlarm;
    CrossTrafficJamAlarm inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    inData.hardCode = "hardCode";
    inData.alarmStatus = 0;
    inData.alarmType = 1;
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    inData.alarmTime = ss.str();
    if (dataUnit->pushO(inData)) {
        printf("伪造数据 CrossTrafficJamAlarm 插入成功\n");
    } else {
        printf("伪造数据 CrossTrafficJamAlarm 插入失败\n");
    }
}

void LocalBusiness::Task_CreateLineupInfoGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto server = local->serverList.find("server1")->second;
    auto dataUnit = &server->dataUnitLineupInfoGather;
    LineupInfoGather inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    inData.hardCode = "hardCode";
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    inData.recordDateTime = ss.str();
    OneLineupInfo item1;
    item1.averageSpeed = 1;
    item1.flow = 2;
    item1.headWay = 3;
    item1.headWayTime = 4;
    item1.laneID = 5;
    item1.occupancy = 6;
    item1.occupancySpace = 7;
    item1.queueLength = 8;
    item1.sumMax = 9;
    item1.sumMid = 10;
    item1.sumMini = 11;
    inData.trafficFlowList.push_back(item1);

    if (dataUnit->pushO(inData)) {
        printf("伪造数据 LineupInfoGather 插入成功\n");
    } else {
        printf("伪造数据 LineupInfoGather 插入失败\n");
    }
}

void LocalBusiness::Task_CreateTrafficFlowGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto server = local->serverList.find("server1")->second;
    auto dataUnit = &server->dataUnitTrafficFlowGather;
    TrafficFlowGather inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    inData.recordDateTime = ss.str();
    OneFlowData item1;
    item1.outAverageSpeed = 1.0;
    item1.inAverageSpeed = 2.0;
    item1.laneDirection = 3;
    item1.flowDirection = 4;
    item1.inCars = 5;
    item1.laneCode = "laneCode";
    item1.outCars = 6;
    item1.queueCars = 7;
    item1.queueLen = 8;
    inData.trafficFlow.push_back(item1);

    if (dataUnit->pushO(inData)) {
        printf("伪造数据 TrafficFlowGather 插入成功\n");
    } else {
        printf("伪造数据 TrafficFlowGather 插入失败\n");
    }
}
