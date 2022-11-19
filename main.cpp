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
        FusionServer::MergeData mergeData;
        if (iter->queueMergeData.pop(mergeData)) {
            FusionData fusionData;
            fusionData.oprNum = random_uuid();
            fusionData.timstamp = mergeData.timestamp;
            fusionData.crossID = iter->watchDatasCrossID;
            //lstObjTarget
            for (int i = 0; i < mergeData.obj.size(); i++) {
                auto iter = mergeData.obj.at(i);
                ObjMix objMix;
                objMix.objID = iter.showID;

                if (iter.objID1 != -INF) {
                    RvWayObject rvWayObject;
                    rvWayObject.wayNo = North;
                    rvWayObject.roID = iter.objID1;
                    rvWayObject.voID = iter.cameraID1;
                    objMix.listRvWayObject.push_back(rvWayObject);
                }
                if (iter.objID2 != -INF) {
                    RvWayObject rvWayObject;
                    rvWayObject.wayNo = East;
                    rvWayObject.roID = iter.objID2;
                    rvWayObject.voID = iter.cameraID2;
                    objMix.listRvWayObject.push_back(rvWayObject);
                }
                if (iter.objID3 != -INF) {
                    RvWayObject rvWayObject;
                    rvWayObject.wayNo = South;
                    rvWayObject.roID = iter.objID3;
                    rvWayObject.voID = iter.cameraID3;
                    objMix.listRvWayObject.push_back(rvWayObject);
                }
                if (iter.objID4 != -INF) {
                    RvWayObject rvWayObject;
                    rvWayObject.wayNo = West;
                    rvWayObject.roID = iter.objID4;
                    rvWayObject.voID = iter.cameraID4;
                    objMix.listRvWayObject.push_back(rvWayObject);
                }

                objMix.objType = iter.objType;
                objMix.objColor = 0;
                objMix.plates = string(iter.plate_number);
                objMix.plateColor = string(iter.plate_color);
                objMix.distance = atof(string(iter.distance).c_str());
                objMix.angle = iter.directionAngle;
                objMix.speed = iter.speed;
                objMix.locationX = iter.locationX;
                objMix.locationY = iter.locationY;
                objMix.longitude = iter.longitude;
                objMix.latitude = iter.latitude;
                objMix.flagNew = iter.flag_new;

                fusionData.lstObjTarget.push_back(objMix);
            }
            if (isSendPicData) {
                fusionData.isHasImage = 1;
                //lstVideos
                for (int i = 0; i < ARRAY_SIZE(mergeData.objInput.roadData); i++) {
                    auto iter = mergeData.objInput.roadData[i];
                    VideoData videoData;
                    videoData.rvHardCode = iter.hardCode;
                    videoData.imageData = iter.imageData;
                    for (auto iter1:iter.listObjs) {
                        VideoTargets videoTargets;
                        videoTargets.cameraObjID = iter1.cameraID;
                        videoTargets.left = iter1.left;
                        videoTargets.top = iter1.top;
                        videoTargets.right = iter1.right;
                        videoTargets.bottom = iter1.bottom;
                        videoData.lstVideoTargets.push_back(videoTargets);
                    }
                    fusionData.lstVideos.push_back(videoData);

                }
            } else {
                fusionData.isHasImage = 0;
                fusionData.lstVideos.resize(0);
            }

            uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgFusionDataWithoutCRC(fusionData, iter->sn_FusionData, deviceNo, pkg);
            iter->sn_FusionData++;
            //存10帧数据到txt
            if (0) {
//            if (saveCount < 10) {
                string filePath = "/mnt/mnt_hd/send/" + to_string(fusionData.timstamp) + ".txt";
                fstream fin;
                fin.open(filePath.c_str(), ios::out | ios::binary | ios::trunc);
                if (fin.is_open()) {
                    fin.write(pkg.body.c_str(), pkg.body.size());
                    fin.flush();
                    fin.close();
                }
//            saveCount++;
//            }
            }

//        Info("sn:%d \t\tfusionData timestamp:%f", pkg.head.sn, fusionData.timstamp);

            if (local->clientList.empty()) {
                Info("client list empty");
                continue;
            }
            timeval start;
            timeval end;
            gettimeofday(&start, nullptr);
            for (int j = 0; j < local->clientList.size(); j++) {
                auto cli = local->clientList.at(j);
                if (cli->isRun) {
                    if (cli->SendToBase(pkg) == -1) {
                        Info("连接到上层，发送消息失败");
                        packetLoss->Fail();
                    } else {
                        packetLoss->Success();
                        gettimeofday(&end, nullptr);
                        int cost = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
//                Info("融合程序连接到上层，发送数据成功,耗时：%d ms", cost);
                    }
                } else {
                    Error("未连接到上层，丢弃消息");
                    packetLoss->Fail();
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
        auto dataUint = iter->dataUnit_TrafficFlows;
        if (!dataUint.o_queue.empty()) {
            TrafficFlows data;
            if (dataUint.o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgTrafficFlowsWithoutCRC(data, dataUint.sn, deviceNo, pkg);
                dataUint.sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendToBase(pkg) == -1) {
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
        auto dataUint = iter->dataUnit_LineupInfoGather;
        if (!dataUint.o_queue.empty()) {
            LineupInfoGather data;
            if (dataUint.o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgLineupInfoGatherWithoutCRC(data, dataUint.sn, deviceNo, pkg);
                dataUint.sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendToBase(pkg) == -1) {
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
        auto dataUint = iter->dataUnit_CrossTrafficJamAlarm;
        if (!dataUint.o_queue.empty()) {
            CrossTrafficJamAlarm data;
            if (dataUint.o_queue.pop(data)) {
                uint32_t deviceNo = stoi(iter->matrixNo.substr(0, 10));
                Pkg pkg;
                PkgCrossTrafficJamAlarmWithoutCRC(data, dataUint.sn, deviceNo, pkg);
                dataUint.sn++;
                if (local->clientList.empty()) {
                    Info("client list empty");
                    continue;
                }
                for (int j = 0; j < local->clientList.size(); j++) {
                    auto cli = local->clientList.at(j);
                    if (cli->isRun) {
                        if (cli->SendToBase(pkg) == -1) {
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
    vector<Timer> timers;

    Timer timerKeep;
    timerKeep.start(1000, std::bind(Task_Keep, &local));
    timers.push_back(timerKeep);

    Timer timerFusionData;
    moniter::PacketLoss packetLoss(1000 * 60);
    timerFusionData.start(1, std::bind(Task_FusionData, &local, &packetLoss));
    timers.push_back(timerFusionData);

    //查看丢包
    Timer timerMontorFusionData;
    timerMontorFusionData.start(60 * 1000, std::bind(MonitorFusionData, &packetLoss));
    timers.push_back(timerMontorFusionData);

    Timer timerTrafficFlows;
    timerTrafficFlows.start(1, std::bind(Task_TrafficFlows, &local));
    timers.push_back(timerTrafficFlows);

    Timer timerLineupInfoGather;
    timerLineupInfoGather.start(1, std::bind(Task_LineupInfoGather, &local));
    timers.push_back(timerLineupInfoGather);

    Timer timerCrossTrafficJamAlarm;
    timerCrossTrafficJamAlarm.start(1, std::bind(Task_CrossTrafficJamAlarm, &local));
    timers.push_back(timerCrossTrafficJamAlarm);

    HttpServerInit(10000, &local);

    StartEocCommon();
    while (true) {
        sleep(10);
    }

    return 0;
}
