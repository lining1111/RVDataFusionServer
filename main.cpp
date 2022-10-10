#include <iostream>
#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include <gflags/gflags.h>
#include <csignal>

#include "client/FusionClient.h"
#include "server/FusionServer.h"
#include "multiView/MultiViewServer.h"
#include "global.h"
#include "httpServer.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include "db/DB.h"

using namespace z_log;

const int INF = 0x7FFFFFFF;

bool isSendPicData;

string dirName;

/**
 * 将服务端得到的融合数据，发送给上层
 * @param p
 */
static void ThreadProcess(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    if (local->server == nullptr || local->client == nullptr) {
        return;
    }

    Info("发送融合后的数据线程 run");
    while (local->isRun) {
        pthread_mutex_lock(&local->server->lockMergeData);
        while (local->server->queueMergeData.empty()) {
            pthread_cond_wait(&local->server->condMergeData, &local->server->lockMergeData);
        }
        auto mergeData = local->server->queueMergeData.front();
        local->server->queueMergeData.pop();
        pthread_mutex_unlock(&local->server->lockMergeData);

        FusionData fusionData;
        fusionData.oprNum = random_uuid();
        fusionData.timstamp = mergeData.timestamp;
        fusionData.crossID = local->server->crossID;
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

//        //存目标数据
//            string fileName = to_string((long) fusionData.timstamp) + ".txt";
//            ofstream inDataFile;
//            string inDataFileName = "mergeData/" + fileName;
//            inDataFile.open(inDataFileName);
//            string content;
//            for (int j = 0; j < fusionData.lstObjTarget.size(); j++) {
//                string split = " ";
//                auto iter = fusionData.lstObjTarget.at(j);
//                content += (to_string(iter.objID) + split +
//                            to_string(iter.objType) + split +
//                            to_string(iter.objColor) + split +
//                            iter.plates + split +
//                            iter.plateColor + split +
//                            to_string(iter.distance) + split +
//                            to_string(iter.angle) + split +
//                            to_string(iter.speed) + split +
//                            to_string(iter.locationX) + split +
//                            to_string(iter.locationY) + split +
//                            to_string(iter.longitude) + split +
//                            to_string(iter.latitude));
//                content += "\n";
//            }
//            if (inDataFile.is_open()) {
//                inDataFile.write((const char *) content.c_str(), content.size());
//                Info("融合数据写入%d", fusionData.lstObjTarget.size());
//                inDataFile.close();
//            } else {
//                Error("打开文件失败:%s", inDataFileName.c_str());
//            }

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

        uint32_t deviceNo = stoi(local->server->matrixNo.substr(0, 10));
        Pkg pkg;
        PkgFusionDataWithoutCRC(fusionData, local->client->sn, deviceNo, pkg);

        Info("sn:%d \t\tfusionData timestamp:%f", pkg.head.sn, fusionData.timstamp);
        local->client->sn++;

        if (local->client->isRun) {
            if (local->client->SendToBase(pkg) == -1) {
                local->client->isRun = false;
                Info("连接到上层，发送消息失败,matrixNo:%d", pkg.head.deviceNO);
            } else {
                Info("融合程序连接到上层，发送数据成功,matrixNo:%d", pkg.head.deviceNO);
            }
        } else {
            Error("未连接到上层，丢弃消息,matrixNo:%d", pkg.head.deviceNO);
        }
    }

    Info("发送融合后的数据线程 exit");
}


static void ThreadProcessMultiView(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    if (local->multiViewServer == nullptr || local->client == nullptr) {
        return;
    }

    Info("发送multiView数据线程 run");
    while (local->isRun) {

        pthread_mutex_lock(&local->multiViewServer->lockObjs);
        while (local->multiViewServer->queueObjs.empty()) {
            pthread_cond_wait(&local->multiViewServer->condObjs, &local->multiViewServer->lockObjs);
        }
        auto objs = local->multiViewServer->queueObjs.front();
        local->multiViewServer->queueObjs.pop();
        pthread_mutex_unlock(&local->multiViewServer->lockObjs);

        uint32_t deviceNo = stoi(local->server->matrixNo.substr(0, 10));
        Pkg pkg;
        PkgTrafficFlowsWithoutCRC(objs, local->client->multiViewSn, deviceNo, pkg);

        Info("multiView sn:%d \t\tfusionData timestamp:%f", pkg.head.sn, objs.timestamp);
        local->client->multiViewSn++;

        if (local->client->isRun) {
            if (local->client->SendToBase(pkg) == -1) {
                local->client->isRun = false;
                Info("multiView连接到上层，发送消息失败,matrixNo:%d", pkg.head.deviceNO);
            } else {
                Info("multiView连接到上层，发送数据成功,matrixNo:%d", pkg.head.deviceNO);
            }
        } else {
            Error("multiView未连接到上层，丢弃消息,matrixNo:%d", pkg.head.deviceNO);
        }
    }

    Info("发送multiView数据线程 exit");
}


/**
 * 查看服务端和客户端状态，并执行断线重连
 * @param p
 */
static void ThreadKeep(void *p) {
    if (p == nullptr) {
        return;
    }

    auto local = (Local *) p;

    if (local->server == nullptr || local->client == nullptr) {
        return;
    }

    Info("客户端和服务端状态检测线程 run");
    while (local->isRun) {
        sleep(10);
        if (!local->server->isRun) {
            local->server->Close();
            if (local->server->Open() == 0) {
                Info("服务端重启");
                local->server->Run();
            }
        }

        if (!local->multiViewServer->isRun) {
            local->multiViewServer->Close();
            if (local->multiViewServer->Open() == 0) {
                Info("多目服务端重启");
                local->multiViewServer->Run();
            }
        }

        if (!local->client->isRun) {
            local->client->Close();
            if (local->client->Open() == 0) {
                Info("客户端重启");
                local->client->Run();
            }
        }
    }
    Info("客户端和服务端状态检测线程 exit");
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
    MultiViewServer *multiViewServer = new MultiViewServer();
    multiViewServer->port = port + 1;

    FusionClient *client = new FusionClient(cloudIp, cloudPort);//端口号和ip依实际情况而变

    Local local;
    local.server = server;
    local.multiViewServer = multiViewServer;
    local.client = client;
    local.isRun = true;

    server->Open();
    server->Run();

    multiViewServer->Open();
    multiViewServer->Run();

    client->Open();
    client->Run();

    thread threadProcess = thread(ThreadProcess, &local);
    threadProcess.detach();

    thread threadProcessMultiView = thread(ThreadProcessMultiView, &local);
    threadProcessMultiView.detach();

    thread threadKeep = thread(ThreadKeep, &local);
    threadKeep.detach();


    HttpServerInit(10000, &local);

    while (true) {
        sleep(10);
    }

    return 0;
}
