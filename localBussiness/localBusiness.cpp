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
#include "common/config.h"

static int saveCountMax = 0;

LocalBusiness *LocalBusiness::m_pInstance = nullptr;

LocalBusiness *LocalBusiness::instance() {
    if (m_pInstance == nullptr) {
        m_pInstance = new LocalBusiness();
    }
    return m_pInstance;
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
    for (auto &iter: serverList) {
        if (iter.second->Open() == 0) {
            iter.second->Run();
        }
    }
    for (auto &iter: clientList) {
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

    //开启伪造数据线程
//    timerCreateFusionData.start(100,std::bind(Task_CreateFusionData,this));
//    addTimerTask("localBusiness timerCreateCrossTrafficJamAlarm",10*1000,std::bind(Task_CreateCrossTrafficJamAlarm,this));
//    addTimerTask("localBusiness timerCreateLineupInfoGather",1000,std::bind(Task_CreateLineupInfoGather,this));
//    timerCreateTrafficFlowGather.start(1000, std::bind(Task_CreateTrafficFlowGather, this));

//    timerCreateAbnormalStopData.start(1000,std::bind(Task_CreateAbnormalStopData,this));
//    timerCreateLongDistanceOnSolidLineAlarm.start(1000,std::bind(Task_CreateLongDistanceOnSolidLineAlarm,this));
//    timerCreateHumanData.start(1000,std::bind(Task_CreateHumanData,this));
}


void LocalBusiness::StopTimerTaskAll() {
    timerKeep.stop();

    timerCreateFusionData.stop();
    timerCreateTrafficFlowGather.stop();
    timerCreateAbnormalStopData.stop();
    timerCreateLongDistanceOnSolidLineAlarm.stop();
    timerCreateHumanData.stop();
}

void LocalBusiness::Task_Keep(void *p) {
    if (p == nullptr) {
        return;
    }
//    LOG(INFO) << __FUNCTION__ << " START";
    auto local = (LocalBusiness *) p;

    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        for (auto &iter: local->serverList) {
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

        for (auto &iter: local->clientList) {
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

uint64_t getSendTimestamp() {
    return (uint64_t) std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

/**
 * 发送输出队列
 * @param client
 * @param msgType
 * @param pkg
 * @return 0:成功，未连接 -2 失败 -1
 */
int LocalBusiness::SendDataUnitO(FusionClient *client, string msgType, Pkg pkg) {

    int ret = 0;

    if (client->isRun) {
        ret = client->SendBase(pkg);
        if (ret < 0) {
            VLOG(2) << msgType << " 发送失败:" << client->server_ip << ":" << client->server_port;
            ret = -1;
        } else {
            VLOG(2) << msgType << " 发送成功:" << client->server_ip << ":" << client->server_port;
            ret = 0;
        }
    } else {
        VLOG(2) << "未连接上层:" << client->server_ip << ":" << client->server_port;
        ret = -2;
    }

    return ret;
}

void LocalBusiness::Task_CreateFusionData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = dataLocal->dataUnitFusionData;
    FusionData inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
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
    auto dataUnit = dataLocal->dataUnitCrossTrafficJamAlarm;
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
    auto dataUnit = dataLocal->dataUnitTrafficFlowGather;
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

void LocalBusiness::Task_CreateAbnormalStopData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = dataLocal->dataUnitAbnormalStopData;
    AbnormalStopData inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    inData.hardCode = "hardCode";
    inData.laneCode = "laneCode";
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    inData.alarmTime = ss.str();
    inData.alarmStatus = 1;
    inData.alarmType = 2;

    if (dataUnit->pushO(inData)) {
        printf("伪造数据 AbnormalStopData 插入成功\n");
    } else {
        printf("伪造数据 AbnormalStopData 插入失败\n");
    }
}

void LocalBusiness::Task_CreateLongDistanceOnSolidLineAlarm(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = dataLocal->dataUnitLongDistanceOnSolidLineAlarm;
    LongDistanceOnSolidLineAlarm inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    inData.hardCode = "hardCode";
    inData.laneCode = "laneCode";
    std::time_t t = std::time(nullptr);
    std::tm *tm = std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    inData.alarmTime = ss.str();
    inData.alarmStatus = 3;
    inData.alarmType = 4;
    inData.latitude = 123.456789;
    inData.longitude = 987.654321;

    if (dataUnit->pushO(inData)) {
        printf("伪造数据 LongDistanceOnSolidLineAlarm 插入成功\n");
    } else {
        printf("伪造数据 LongDistanceOnSolidLineAlarm 插入失败\n");
    }
}

void LocalBusiness::Task_CreateHumanData(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    //只往第1个server放数据
    auto dataLocal = Data::instance();
    auto dataUnit = dataLocal->dataUnitHumanData;
    HumanDataGather inData;
    inData.oprNum = random_uuid();
    inData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    inData.crossID = "crossID";
    inData.hardCode = "hardCode";
    HumanDataGather_deviceListItem item;
    item.deviceCode = "deviceCode";
    item.direction = 5;
    item.detectDirection = 6;
    item.bicycleNum = 100;
    item.humanType = 101;
    item.humanNum = 102;
    inData.deviceList.push_back(item);

    if (dataUnit->pushO(inData)) {
        printf("伪造数据 HumanData 插入成功\n");
    } else {
        printf("伪造数据 HumanData 插入失败\n");
    }
}


