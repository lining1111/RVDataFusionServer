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
#include "glog/logging.h"

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
            LOG(INFO) << "create path:" << dirName1;
        else
            LOG(INFO) << "create path failed! error code:" << isCreate1;
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

    addTimerTask("localBusiness timerKeep", 1000*3, std::bind(Task_Keep, this));

    addTimerTask("localBusiness timerTask_FusionData", 100,
                 std::bind(Task_FusionData, this));
    addTimerTask("localBusiness timerTask_TrafficFlowGather", 1000,
                 std::bind(Task_TrafficFlowGather, this));
    addTimerTask("localBusiness timerTask_CrossTrafficJamAlarm", 1000,
                 std::bind(Task_CrossTrafficJamAlarm, this));
    addTimerTask("localBusiness timerTask_IntersectionOverflowAlarm", 1000,
                 std::bind(Task_IntersectionOverflowAlarm, this));
    addTimerTask("localBusiness timerTask_InWatchData_1_3_4", 1000,
                 std::bind(Task_InWatchData_1_3_4, this));
    addTimerTask("localBusiness timerTask_InWatchData_2", 1000,
                 std::bind(Task_InWatchData_2, this));

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
                    LOG(WARNING) << "服务端:" << iter.first << " 重启";
                    iter.second->Run();
                }
            }
        }

        for (auto &iter:local->clientList) {
            if (!iter.second->isRun) {
                iter.second->Close();
                if (iter.second->Open() == 0) {
                    LOG(WARNING) << "客户端:" << iter.first << " 重启";
                    iter.second->Run();
                }
            }
        }
    }
}

int LocalBusiness::SendDataUnitO(LocalBusiness *local, string msgType, Pkg pkg, uint64_t timestamp, bool isSave) {
    //存发送
    if (isSave) {
        string dirName1 = savePath + msgType;
        int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (!isCreate1)
            LOG(INFO) << "create path:" << dirName1;
        else
            LOG(INFO) << "create path failed! error code:" << isCreate1;

        string fileName = savePath + msgType + "/" + to_string(timestamp) + ".txt";
        ofstream file;
        file.open(fileName);
        if (file.is_open()) {
            file.write(pkg.body.data(), pkg.body.size());
            file.flush();
            file.close();
        }
    }

    if (local->clientList.empty()) {
        LOG(ERROR) << "client list empty";
        return -1;
    }
    int ret = 0;
    for (auto &iter1:local->clientList) {
        auto cli = iter1.second;
        if (cli->isRun) {
            LOG(INFO) << "发送到上层" << cli->server_ip << ":" << cli->server_port
                      << "消息:" << msgType << ",matrixNo:" << pkg.head.deviceNO;
            if (cli->SendBase(pkg) == -1) {
                LOG(INFO) << msgType << " 发送失败:" << cli->server_ip << ":" << cli->server_port;
                ret = -1;
            } else {
                LOG(INFO) << msgType << " 发送成功:" << cli->server_ip << ":" << cli->server_port;
            }
        } else {
            LOG(INFO) << "未连接上层:" << cli->server_ip << ":" << cli->server_port;
            ret = -1;
        }
    }
    return ret;
}


void LocalBusiness::Task_CrossTrafficJamAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "CrossTrafficJamAlarm";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitCrossTrafficJamAlarm;

            CrossTrafficJamAlarm data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
            }
        }
    }
}

void LocalBusiness::Task_IntersectionOverflowAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "IntersectionOverflowAlarm";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitIntersectionOverflowAlarm;

            IntersectionOverflowAlarm data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
            }
        }
    }
}

void LocalBusiness::Task_TrafficFlowGather(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "TrafficFlowGather";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitTrafficFlowGather;

            TrafficFlowGather data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
            }
        }
    }
}

void LocalBusiness::Task_FusionData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;

    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }
    if (local->isRun) {
        string msgType = "FusionData";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitFusionData;

            FusionData data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timstamp);
            }
        }
    }
}

void LocalBusiness::Task_InWatchData_1_3_4(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "InWatchData_1_3_4";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitInWatchData_1_3_4;

            InWatchData_1_3_4 data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
            }
        }
    }
}

void LocalBusiness::Task_InWatchData_2(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "InWatchData_2";
        for (auto &iter: local->serverList) {
            auto server = iter.second;
            auto dataUnit = &server->dataUnitInWatchData_2;

            InWatchData_2 data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
            }
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
