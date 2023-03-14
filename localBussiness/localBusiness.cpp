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
#include "data/Data.h"

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

void LocalBusiness::AddServer(string name, int port) {
    FusionServer *server = new FusionServer(port);
    server->port = port;
    serverList.insert(make_pair(name, server));
}

void LocalBusiness::AddClient(string name, string cloudIp, int cloudPort) {
    FusionClient *client = new FusionClient(cloudIp, cloudPort);//端口号和ip依实际情况而变
    clientList.insert(make_pair(name, client));
}


void LocalBusiness::Run() {
    isRun = true;
    for (auto &iter:serverList) {
        if (iter.second->Open() == 0) {
            iter.second->Run();
        }
    }
    for (auto &iter:clientList) {
        if (iter.second->Open() == 0) {
            iter.second->Run();
        }

    }
    if (0) {
        string dirName1 = savePath;
        int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (!isCreate1)
            LOG(INFO) << "create path:" << dirName1;
        else
            LOG(INFO) << "create path failed! error _code:" << isCreate1;
    }

    StartTimerTask();
}

void LocalBusiness::StartTimerTask() {
    timerKeep.start(1000 * 3, std::bind(Task_Keep, this));
    timerFusionData.start(100, std::bind(Task_FusionData, this));
    timerTrafficFlowGather.start(1000, std::bind(Task_TrafficFlowGather, this));
    timerCrossTrafficJamAlarm.start(1000, std::bind(Task_CrossTrafficJamAlarm, this));
    timerIntersectionOverflowAlarm.start(1000, std::bind(Task_IntersectionOverflowAlarm, this));
    timerInWatchData_1_3_4.start(1000, std::bind(Task_InWatchData_1_3_4, this));
    timerInWatchData_2.start(1000, std::bind(Task_InWatchData_2, this));
    timerStopLinePassData.start(1000, std::bind(Task_StopLinePassData, this));

    //开启伪造数据线程
//    timerCreateFusionData.start(100,std::bind(Task_CreateFusionData,this));
//    addTimerTask("localBusiness timerCreateCrossTrafficJamAlarm",10*1000,std::bind(Task_CreateCrossTrafficJamAlarm,this));
//    addTimerTask("localBusiness timerCreateLineupInfoGather",1000,std::bind(Task_CreateLineupInfoGather,this));
//    addTimerTask("localBusiness timerCreateTrafficFlowGather",1000,std::bind(Task_CreateTrafficFlowGather,this));
}


void LocalBusiness::StopTimerTaskAll() {
    timerKeep.stop();
    timerFusionData.stop();
    timerTrafficFlowGather.stop();
    timerCrossTrafficJamAlarm.stop();
    timerIntersectionOverflowAlarm.stop();
    timerInWatchData_1_3_4.stop();
    timerInWatchData_2.stop();
    timerStopLinePassData.stop();
}

void LocalBusiness::Task_Keep(void *p) {
    if (p == nullptr) {
        return;
    }
    LOG(INFO) << __FUNCTION__ << " START";
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
                } else {
                    LOG(WARNING) << "服务端:" << iter.first << " 重启失败";
                }
            }
        }

        for (auto &iter:local->clientList) {
            if (!iter.second->isRun) {
                iter.second->Close();
                if (iter.second->Open() == 0) {
                    LOG(WARNING) << "客户端:" << iter.first << " 重启";
                    iter.second->Run();
                } else {
                    LOG(WARNING) << "客户端:" << iter.first << " 重启失败";
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
            VLOG(2) << "create path:" << dirName1;
        else
            VLOG(2) << "create path failed! error _code:" << isCreate1;

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
            VLOG(3) << "发送到上层" << cli->server_ip << ":" << cli->server_port
                    << "消息:" << msgType << ",matrixNo:" << pkg.head.deviceNO;
            if (cli->SendBase(pkg) == -1) {
                VLOG(3) << msgType << " 发送失败:" << cli->server_ip << ":" << cli->server_port;
                ret = -1;
            } else {
                VLOG(3) << msgType << " 发送成功:" << cli->server_ip << ":" << cli->server_port;
            }
        } else {
            VLOG(3) << "未连接上层:" << cli->server_ip << ":" << cli->server_port;
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

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitCrossTrafficJamAlarm;

        CrossTrafficJamAlarm data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
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

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitIntersectionOverflowAlarm;

        IntersectionOverflowAlarm data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
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

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitTrafficFlowGather;

        TrafficFlowGather data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
        }
    }
}

void LocalBusiness::Task_FusionData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    LOG(INFO) << __FUNCTION__ << " START";
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }
    if (local->isRun) {
        string msgType = "FusionData";
        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitFusionData;

        FusionData data;
        if (dataUnit->popO(data)) {
            printf("-----fusiondata crossid:%s,lstobjTarget size:%d,lstVideos size:%d\n",
                   data.crossID.c_str(),data.lstObjTarget.size(),data.lstVideos.size());
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timstamp);
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

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitInWatchData_1_3_4;

        InWatchData_1_3_4 data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
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

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitInWatchData_2;

        InWatchData_2 data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
        }
    }
}

void LocalBusiness::Task_StopLinePassData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        string msgType = "StopLinePassData";

        auto dataLocal = Data::instance();
        auto dataUnit = &dataLocal->dataUnitStopLinePassData;

        StopLinePassData data;
        if (dataUnit->popO(data)) {
            uint32_t deviceNo = stoi(dataLocal->matrixNo.substr(0, 10));
            Pkg pkg;
            data.PkgWithoutCRC(dataUnit->sn, deviceNo, pkg);
            dataUnit->sn++;
            SendDataUnitO(local, msgType, pkg, (uint64_t) data.timestamp);
        }
    }
}

void LocalBusiness::Task_CreateFusionData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = &dataLocal->dataUnitFusionData;
    FusionData inData;
    inData.oprNum = random_uuid();
    inData.timstamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    if (dataUnit->pushO(inData)) {
        printf("伪造数据 CrossFusionData 插入成功\n");
    } else {
        printf("伪造数据 CrossFusionData 插入失败\n");
    }
}


void LocalBusiness::Task_CreateCrossTrafficJamAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = &dataLocal->dataUnitCrossTrafficJamAlarm;
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
    auto dataLocal = Data::instance();
    auto dataUnit = &dataLocal->dataUnitTrafficFlowGather;
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
