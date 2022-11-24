//
// Created by lining on 11/22/22.
//

#include <functional>
#include "localBusiness.h"
#include <arpa/inet.h>

#include "log/Log.h"

using namespace z_log;

LocalBusiness::LocalBusiness() {
    packetLossFusionData = new moniter::PacketLoss(1000 * 60);
}

LocalBusiness::~LocalBusiness() {
    StopTimerTaskAll();

    for (auto iter = clientList.begin(); iter != clientList.end();) {
        delete iter->second;
        iter = clientList.erase(iter);
    }
    for (auto iter = serverList.begin(); iter != serverList.end();) {
        delete iter->second;
        iter = serverList.erase(iter);
    }

    delete packetLossFusionData;
}

void LocalBusiness::AddServer(string name, int port, bool isMerge) {
    FusionServer *server = new FusionServer(port, isMerge);
    server->port = port;
    serverList.insert(make_pair(name, server));
}

void LocalBusiness::AddClient(string name, string cloudIp, int cloudPort, void *super) {
    FusionClient *client = new FusionClient(cloudIp, cloudPort, nullptr);//端口号和ip依实际情况而变
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
    addTimerTask("localBusiness timerFusionData", 100, std::bind(Task_FusionData, this));
    //查看丢包
    addTimerTask("localBusiness timerMonitorFusionData", 1000 * 60, std::bind(MonitorPacketLoss, this));

    addTimerTask("localBusiness timerTrafficFlows", 100, std::bind(Task_TrafficFlows, this));
    addTimerTask("localBusiness timerLineupInfoGather", 100, std::bind(Task_LineupInfoGather, this));
    addTimerTask("localBusiness timerCrossTrafficJamAlarm", 1000 * 10, std::bind(Task_CrossTrafficJamAlarm, this));
    addTimerTask("localBusiness timerMultiViewCarTracks", 66, std::bind(Task_MultiViewCarTracks, this));
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
                    Info("服务端 %s 重启", iter.first.c_str());
                    iter.second->Run();
                }
            }
        }

        for (auto &iter:local->clientList) {
            if (!iter.second->isRun) {
                iter.second->Close();
                if (iter.second->Open() == 0) {
                    Info("客户端 %s 重启", iter.first.c_str());
                    iter.second->Run();
                }
            }
        }
    }
}

void LocalBusiness::Task_MultiViewCarTracks(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (LocalBusiness *) p;

    string msgType = "MultiViewCarTracks";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitMultiViewCarTracks;
        if (!dataUnit->emptyO()) {
            MultiViewCarTracks data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgMultiViewCarTracksWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }

                for (auto &iter1:local->clientList) {
                    auto cli = iter1.second;
                    if (cli->isRun) {
                        Info("server %s 发送到上层 %s,消息%s,matrixNo:%d",
                             iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                        if (cli->SendBase(pkg) == -1) {
                            Error("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                        } else {
                            Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        }
                    } else {
                        Error(":未连接上层%s", cli->server_ip.c_str());
                    }
                }
            }
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
        if (!dataUnit->emptyO()) {
            CrossTrafficJamAlarm data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgCrossTrafficJamAlarmWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }

                for (auto &iter1:local->clientList) {
                    auto cli = iter1.second;
                    if (cli->isRun) {
                        Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                             iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                        if (cli->SendBase(pkg) == -1) {
                            Error("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                        } else {
                            Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        }
                    } else {
                        Error(":未连接上层%s", cli->server_ip.c_str());
                    }
                }
            }
        }
    }
}

void LocalBusiness::Task_LineupInfoGather(void *p) {
    if (p == nullptr) {
        return;
    }
    string msgType = "LineupInfoGather";
    auto local = (LocalBusiness *) p;

    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitLineupInfoGather;
        if (!dataUnit->emptyO()) {
            LineupInfoGather data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgLineupInfoGatherWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }

                for (auto &iter1:local->clientList) {
                    auto cli = iter1.second;
                    if (cli->isRun) {
                        Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                             iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                        if (cli->SendBase(pkg) == -1) {
                            Error("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                        } else {
                            Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        }
                    } else {
                        Error("未连接上层%s", cli->server_ip.c_str());
                    }
                }
            }
        }
    }
}


void LocalBusiness::Task_TrafficFlows(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (LocalBusiness *) p;
    string msgType = "TrafficFlows";
    for (auto &iter: local->serverList) {
        auto server = iter.second;
        auto dataUnit = &server->dataUnitTrafficFlows;
        if (!dataUnit->emptyO()) {
            TrafficFlows data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgTrafficFlowsWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }

                for (auto &iter1:local->clientList) {
                    auto cli = iter1.second;
                    if (cli->isRun) {
                        Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                             iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                        if (cli->SendBase(pkg) == -1) {
                            Error("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                        } else {
                            Info("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                        }
                    } else {
                        Error("未连接上层%s", cli->server_ip.c_str());
                    }
                }
            }
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
        if (!dataUnit->emptyO()) {
            FusionData data;
            if (dataUnit->popO(data)) {
                uint32_t deviceNo = stoi(server->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgFusionDataWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }

                for (auto &iter1:local->clientList) {
                    auto cli = iter1.second;
                    if (cli->isRun) {
                        Info("server %s 发送到上层%s,消息%s,matrixNo:%d",
                             iter.first.c_str(), iter1.first.c_str(), msgType.c_str(), pkg.head.deviceNO);
                        if (cli->SendBase(pkg) == -1) {
                            Error("%d发送%s失败", pkg.head.cmd, cli->server_ip.c_str());
                            local->packetLossFusionData->Fail();
                        } else {
                            Error("%d发送%s成功", pkg.head.cmd, cli->server_ip.c_str());
                            local->packetLossFusionData->Success();
                        }
                    } else {
                        Error("未连接上层%s", cli->server_ip.c_str());
                        local->packetLossFusionData->Fail();
                    }
                }
            }
        }
    }
}

void LocalBusiness::MonitorPacketLoss(void *p) {
    if (p == nullptr) {
        return;
    }
    auto local = (LocalBusiness *) p;
    Info("FusionData 当前丢包率:%f", local->packetLossFusionData->ShowLoss());
}