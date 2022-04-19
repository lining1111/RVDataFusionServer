#include <iostream>
#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include <gflags/gflags.h>

#include "client/FusionClient.h"
#include "server/FusionServer.h"

using namespace log;

typedef struct {
    FusionServer *server;
    FusionClient *client;
    bool isRun;
} Local;

int sn = 0;
uint32_t deviceNo = 0x12345678;

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
        usleep(10);
        if (local->server->queueMergeData.Size() == 0) {
            continue;
        }
        do {
            auto mergeData = local->server->queueMergeData.PopFront();
            FusionData fusionData;
            fusionData.oprNum = random_uuid();
            fusionData.isHasImage = 0;
            fusionData.imageData = "";
            fusionData.timstamp = mergeData.timestamp;
            fusionData.crossID = local->server->crossID;
            for (int i = 0; i < mergeData.obj.size(); i++) {
                auto iter = mergeData.obj.at(i);
                ObjMix objMix;
                objMix.objID = iter.showID;
                objMix.cameraObjID = iter.showID;
                objMix.objType = iter.objType;
                objMix.objColor = 0;
                objMix.plates = string(iter.plate_number);
                objMix.plateColor = string(iter.plate_color);
                objMix.left = iter.left;
                objMix.top = iter.top;
                objMix.right = iter.top;
                objMix.bottom = iter.bottom;
                objMix.distance = atof(string(iter.distance).c_str());
                objMix.angle = iter.directionAngle;
                objMix.speed = iter.speed;
                objMix.locationX = iter.locationX;
                objMix.locationY = iter.locationY;
                objMix.longitude = iter.longitude;
                objMix.latitude = iter.latitude;

                fusionData.lstObjTarget.push_back(objMix);
            }

            uint32_t deviceNo = stoi(local->server->matrixNo.substr(0, 10));
            Pkg pkg;
            PkgFusionDataWithoutCRC(fusionData, local->client->sn, deviceNo, pkg);
            local->client->sn++;
            sn++;

            if (local->client->isRun) {
//                local->client->Send(pkg);
                if (local->client->SendToBase(pkg) == -1) {
                    local->client->isRun = false;
                }
                Info("连接到上层，发送消息,matrixNo:%d", pkg.head.deviceNO);
                Info("连接到上层，发送消息:%s", pkg.body.c_str());
            } else {
                Error("未连接到上层，丢弃消息,matrixNo:%d", pkg.head.deviceNO);
                Error("未连接到上层，丢弃消息:%s", pkg.body.c_str());
            }

        } while (local->server->queueMergeData.Size() > 0);
    }

    Info("发送融合后的数据线程 exit");
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
        usleep(10 * 1000);
        if (!local->server->isRun) {
            Info("服务端重启");
            local->server->Close();
            local->server->Open();
            local->server->Run();
        }

        if (!local->client->isRun) {
            Info("客户端重启");
            local->client->Close();
            local->client->Open();
            local->client->Run();
        }
    }
    Info("客户端和服务端状态检测线程 exit");
}

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "10.110.60.121", "云端ip，默认 10.110.60.121");
DEFINE_int32(cloudPort, 9200, "云端端口号，默认9200");

int main(int argc, char **argv) {

    google::SetVersionString(PROJECT_VERSION);
    google::ParseCommandLineFlags(&argc, &argv, true);
    log::init();
    uint16_t port = FLAGS_port;
    string cloudIp = FLAGS_cloudIp;
    uint16_t cloudPort = FLAGS_cloudPort;


    //启动融合服务端接受多个单路RV数据;启动融合客户端连接上层，将融合后的数据发送到上层;启动状态监测线程，检查服务端和客户端状态，断开后重连
    FusionServer *server = new FusionServer();
    server->port = port;
    FusionClient *client = new FusionClient(cloudIp, cloudPort);//端口号和ip依实际情况而变

    Local local;
    local.server = server;
    local.client = client;
    local.isRun = true;

    server->Open();
    server->Run();

    client->Open();
    client->Run();

    thread threadProcess = thread(ThreadProcess, &local);
    threadProcess.detach();

    thread threadKeep = thread(ThreadKeep, &local);
    threadKeep.detach();

    while (true) {
        sleep(1);
    }

    return 0;
}
