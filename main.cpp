#include <iostream>
#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include "ParseFlag.h"

#include "client/FusionClient.h"
#include "server/FusionServer.h"

using namespace log;

typedef struct {
    FusionServer *server;
    FusionClient *client;
    bool isRun;
} Local;


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
                objMix.distance = string(iter.distance);
                objMix.angle = iter.directionAngle;
                objMix.speed = iter.speed;
                objMix.locationX = iter.locationX;
                objMix.locationY = iter.locationY;
                objMix.longitude = 0.0;
                objMix.latitude = 0.0;

                fusionData.lstObjTarget.push_back(objMix);
            }

            Pkg pkg;
            PkgFusionDataWithoutCRC(fusionData, local->client->sn, local->client->deviceNo, pkg);
            local->client->sn++;

            if (local->client->isRun) {
                local->client->Send(pkg);
            } else {
                Info("未连接到上层，丢弃消息:%s", pkg.body.c_str());
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


int main(int argc, char **argv) {

    map<string, string> use;
    use["-a"] = "设置a";
    use["-b"] = "设置b";
    use["-cd"] = "设置cd";
    use["-h"] = "显示帮助信息";

    ParseFlag *parseFlag = new ParseFlag(use, ParseFlag::SinglePole);
    if (argc == 2 && string(argv[1]) == "--version") {

#if defined( PROJECT_VERSION )
        cout << "" << PROJECT_VERSION << endl;

#else
        cout << "build date:" << __DATE__ << endl;
#endif
        return 0;
    } else if (argc == 2 && string(argv[1]) == "-h") {
        parseFlag->ShowHelp();
        return 0;
    } else {
        parseFlag->Parse(argc, argv);
    }


    //启动融合服务端接受多个单路RV数据;启动融合客户端连接上层，将融合后的数据发送到上层;启动状态监测线程，检查服务端和客户端状态，断开后重连
    FusionServer *server = new FusionServer();
    FusionClient *client = new FusionClient("127.0.0.1", 1234);//端口号和ip依实际情况而变

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

//    log::init();
//    int x = 4;
//    printf("x = %d\n", x);
//
//    Fatal("x = %d", x);




    return 0;
}
