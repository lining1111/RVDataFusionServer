#include <iostream>
#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include <gflags/gflags.h>
#include <csignal>

#include "timeTask.hpp"
#include "client/FusionClient.h"
#include "server/FusionServer.h"
#include "global.h"
#include "httpServer/httpServer.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include "db/DB.h"
#include "eoc/Eoc.h"
#include "monitor/PacketLoss.hpp"

using namespace z_log;

const int INF = 0x7FFFFFFF;

bool isSendPicData;

string dirName;
int saveCount = 0;

/**
 * 将服务端得到的融合数据，发送给上层
 * @param p
 */
static void Task_FusionData(void *p, moniter::PacketLoss *packetLoss) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    for (int i = 0; i < local->serverList.size(); i++) {
        auto iter = local->serverList.at(i);
        auto dataUnit = &iter->dataUnitFusionData;
        unique_lock<mutex> lck(dataUnit->mtx);
        if (!dataUnit->o_queue.empty()) {
            FusionData data;
            if (dataUnit->o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgFusionDataWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendBase(pkg) == -1) {
                            Info("server %d FusionData连接到上层%d，发送消息失败,matrixNo:%d", i, j, pkg.head.deviceNO);
                            packetLoss->Fail();
                        } else {
//                Info("server %d FusionData连接到上层%d，发送数据成功,matrixNo:%d", i,,j,pkg.head.deviceNO);
                            packetLoss->Success();
                        }
                    } else {
                        Error("server %d FusionData未连接到上层%d，丢弃消息,matrixNo:%d", i, j, pkg.head.deviceNO);
                        packetLoss->Fail();
                    }
                }
            }
        }
    }
}

static void MonitorFusionData(moniter::PacketLoss *packetLoss) {
    Info("%s 当前丢包率:%f", __FUNCTION__, packetLoss->ShowLoss());
}


static void Task_TrafficFlows(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    for (int i = 0; i < local->serverList.size(); i++) {
        auto iter = local->serverList.at(i);
        auto dataUnit = &iter->dataUnitTrafficFlows;
        unique_lock<mutex> lck(dataUnit->mtx);
        if (!dataUnit->o_queue.empty()) {
            TrafficFlows data;
            if (dataUnit->o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgTrafficFlowsWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendBase(pkg) == -1) {
                            Info("server %d TrafficFlows连接到上层%d，发送消息失败,matrixNo:%d", i, j, pkg.head.deviceNO);
                        } else {
//                Info("server %d TrafficFlows连接到上层%d，发送数据成功,matrixNo:%d", i,,j,pkg.head.deviceNO);
                        }
                    } else {
                        Error("server %d TrafficFlows未连接到上层%d，丢弃消息,matrixNo:%d", i, j, pkg.head.deviceNO);
                    }
                }
            }
        }
    }
}

static void Task_LineupInfoGather(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    for (int i = 0; i < local->serverList.size(); i++) {
        auto iter = local->serverList.at(i);
        auto dataUnit = &iter->dataUnitLineupInfoGather;
        unique_lock<mutex> lck(dataUnit->mtx);
        if (!dataUnit->o_queue.empty()) {
            LineupInfoGather data;
            if (dataUnit->o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgLineupInfoGatherWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendBase(pkg) == -1) {
                            Info("server %d LineupInfoGather连接到上层%d，发送消息失败,matrixNo:%d", i, j, pkg.head.deviceNO);
                        } else {
//                Info("server %d LineupInfoGather连接到上层%d，发送数据成功,matrixNo:%d", i,j,pkg.head.deviceNO);
                        }
                    } else {
                        Error("server %d LineupInfoGather未连接到上层%d，丢弃消息,matrixNo:%d", i, j, pkg.head.deviceNO);
                    }
                }
            }
        }
    }
}

static void Task_CrossTrafficJamAlarm(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    for (int i = 0; i < local->serverList.size(); i++) {
        auto iter = local->serverList.at(i);
        auto dataUnit = &iter->dataUnitCrossTrafficJamAlarm;
        unique_lock<mutex> lck(dataUnit->mtx);
        if (!dataUnit->o_queue.empty()) {
            CrossTrafficJamAlarm data;
            if (dataUnit->o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgCrossTrafficJamAlarmWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendBase(pkg) == -1) {
                            Info("server %d CrossTrafficJamAlarm连接到上层%d，发送消息失败,matrixNo:%d", i, j,
                                 pkg.head.deviceNO);
                        } else {
//                Info("server %d CrossTrafficJamAlarm连接到上层%d，发送数据成功,matrixNo:%d", i,j,pkg.head.deviceNO);
                        }
                    } else {
                        Error("server %d CrossTrafficJamAlarm未连接到上层%d，丢弃消息,matrixNo:%d", i, j, pkg.head.deviceNO);
                    }
                }
            }
        }
    }
}

static void Task_MultiViewCarTracks(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    for (int i = 0; i < local->serverList.size(); i++) {
        auto iter = local->serverList.at(i);
        auto dataUnit = &iter->dataUnitMultiViewCarTracks;
        unique_lock<mutex> lck(dataUnit->mtx);
        if (!dataUnit->o_queue.empty()) {
            MultiViewCarTracks data;
            if (dataUnit->o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgMultiViewCarTracksWithoutCRC(data, dataUnit->sn, deviceNo, pkg);
                dataUnit->sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendBase(pkg) == -1) {
                            Info("server %d CrossTrafficJamAlarm连接到上层%d，发送消息失败,matrixNo:%d", i, j,
                                 pkg.head.deviceNO);
                        } else {
//                Info("server %d CrossTrafficJamAlarm连接到上层%d，发送数据成功,matrixNo:%d", i,j,pkg.head.deviceNO);
                        }
                    } else {
                        Error("server %d CrossTrafficJamAlarm未连接到上层%d，丢弃消息,matrixNo:%d", i, j, pkg.head.deviceNO);
                    }
                }
            }
        }
    }
}


/**
 * 查看服务端和客户端状态，并执行断线重连
 * @param p
 */
static void Task_Keep(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    if (local->serverList.empty() || local->clientList.empty()) {
        return;
    }

    if (local->isRun) {
        for (int i = 0; i < local->serverList.size(); i++) {
            auto iter = local->serverList.at(i);
            if (!iter->isRun) {
                iter->Close();
                if (iter->Open() == 0) {
                    Info("服务端 %d 重启", i);
                    iter->Run();
                }
            }
        }

        for (int i = 0; i < local->clientList.size(); i++) {
            auto iter = local->clientList.at(i);
            if (!iter->isRun) {
                iter->Close();
                if (iter->Open() == 0) {
                    Info("客户端 %d 重启", i);
                    iter->Run();
                }
            }
        }
    }
}

int signalIgnpipe() {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) < 0) {
        Error("call sigaction fail, %s\n", strerror(errno));
        return errno;
    }

    return 0;
}


typedef map<string, Timer *> TimerTasks;

void addTimerTask(TimerTasks *timerTasks, string name, uint64_t timeval_ms, std::function<void()> task) {
    Timer *timer = new Timer();
    timer->start(timeval_ms, task);
    timerTasks->insert(make_pair(name, timer));
}

moniter::PacketLoss packetLoss(1000 * 60);

void StartTimerTask(TimerTasks *timerTasks, Local *local) {

    addTimerTask(timerTasks, "main timerKeep", 1000, std::bind(Task_Keep, local));
    addTimerTask(timerTasks, "main timerFusionData", 100, std::bind(Task_FusionData, local, &packetLoss));
    //查看丢包
    addTimerTask(timerTasks, "main timerMontorFusionData", 1000 * 60, std::bind(MonitorFusionData, &packetLoss));

    addTimerTask(timerTasks, "main timerTrafficFlows", 100, std::bind(Task_TrafficFlows, local));
    addTimerTask(timerTasks, "main timerLineupInfoGather", 100, std::bind(Task_LineupInfoGather, local));
    addTimerTask(timerTasks, "main timerCrossTrafficJamAlarm", 1000 * 10, std::bind(Task_CrossTrafficJamAlarm, local));
    addTimerTask(timerTasks, "main timerMultiViewCarTracks", 66, std::bind(Task_MultiViewCarTracks, local));
}

void deleteTimerTask(TimerTasks *timerTasks, string name) {
    auto timerTask = timerTasks->find(name);
    if (timerTask != timerTasks->end()) {
        delete timerTask->second;
        timerTasks->erase(timerTask++);
    }
}

void StopTimerTaskAll(TimerTasks *timerTasks) {
    auto iter = timerTasks->begin();
    while (iter != timerTasks->end()) {
        delete iter->second;
        iter = timerTasks->erase(iter);
    }
}

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "10.110.25.149", "云端ip，默认 10.110.25.149");
DEFINE_int32(cloudPort, 7890, "云端端口号，默认7890");
DEFINE_bool(isSendPicData, true, "是否发送图像数据，默认发送true");
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");

int main(int argc, char **argv) {


    //创建以picsocknum为名的文件夹
//    for (int i = 0; i < 4; i++) {
//        dirName = "merge" + to_string(3);
//        int isCreate = mkdir(dirName.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
//        if (!isCreate)
//            Info("create path:%s\n", dirName.c_str());
//        else
//            Info("create path failed! error code : %d \n", isCreate);
//    }
    if (0) {
        string dirName1 = "mergeData";
        int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (!isCreate1)
            Info("create path:%s\n", dirName1.c_str());
        else
            Info("create path failed! error code : %d \n", isCreate1);
    }

    google::SetVersionString(PROJECT_VERSION);
    google::ParseCommandLineFlags(&argc, &argv, true);
    z_log::init();
    uint16_t port = FLAGS_port;
    string cloudIp = FLAGS_cloudIp;
    uint16_t cloudPort = FLAGS_cloudPort;
    isSendPicData = FLAGS_isSendPicData;
    bool isMerge = FLAGS_isMerge;

    signalIgnpipe();
    //启动融合服务端接受多个单路RV数据;启动融合客户端连接上层，将融合后的数据发送到上层;启动状态监测线程，检查服务端和客户端状态，断开后重连
    FusionServer *server = new FusionServer(isMerge);
    server->port = port;

    //多目服务端
    FusionServer *multiviewServer = new FusionServer(isMerge);
    multiviewServer->port = port + 1;

    FusionClient *client = new FusionClient(cloudIp, cloudPort, nullptr);//端口号和ip依实际情况而变

    Local local;

    local.serverList.push_back(server);
    local.serverList.push_back(multiviewServer);
    local.clientList.push_back(client);
    local.isRun = true;

    for (auto &iter:local.serverList) {
        iter->Open();
        iter->Run();
    }
    for (auto &iter:local.clientList) {
        iter->Open();
        iter->Run();
    }

    //开启发送定时任务
    TimerTasks timerTasks;
    StartTimerTask(&timerTasks, &local);

    HttpServerInit(10000, &local);

    StartEocCommon();
    while (true) {
        sleep(10);
    }

    return 0;
}
