//
// Created by lining on 2/18/22.
//

#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include "server/FusionServer.h"
#include "log/Log.h"
#include "merge/merge.h"

using namespace log;

int main(int argc, char **argv) {

    FusionServer *server = new FusionServer();

    server->Open();
    server->Run();

    while (server->isRun.load()) {
        usleep(1);
        //循环出读server各个client内的WatchData队列
//        if (server->vector_client.empty()) {
//            Info("客户端队列为空");
//            continue;
//        }
//        cout << "客户端数量：" << to_string(server->vector_client.size()) << endl;

//        //读取客户端的queueWatchData
//        pthread_mutex_lock(&server->lock_vector_client);
//        if (server->vector_client.empty()) {
//            pthread_cond_wait(&server->cond_vector_client, &server->lock_vector_client);
//        }
//        for (auto iter: server->vector_client) {
//            if (iter->queueWatchData.size() == 0) {
//                Info("此客户端%d-%s中 watchData为空", iter->sock, inet_ntoa(iter->clientAddr.sin_addr));
//            } else {
//                //取出所有消息，并打印部分信息
//                do {
//                    WatchData watchData;
//                    pthread_mutex_lock(&iter->lockWatchData);
//                    if (iter->queueWatchData.empty()) {
//                        pthread_cond_wait(&iter->condWatchData, &iter->lockWatchData);
//                    }
//
//                    watchData = iter->queueWatchData.front();
//                    iter->queueWatchData.pop();
//
//                    Info("WatchData:%s-%f ip:%s", watchData.oprNum.c_str(), watchData.RecordDateTime,
//                         watchData.cameraIp.c_str());
//                    pthread_cond_broadcast(&iter->condWatchData);
//                    pthread_mutex_unlock(&iter->lockWatchData);
//
//                } while (iter->queueWatchData.size() > 0);
//
//            }
//        }
//
//        pthread_mutex_unlock(&server->lock_vector_client);
        //读取服务端的queueMergeData
        if (server->queueMergeData.Size() == 0) {
//            Info("server 融合数据队列为空");
            continue;
        }
        FusionServer::MergeData mergeData = server->queueMergeData.PopFront();
        Info("server mergeData: timestamp:%f,size:%d",
             mergeData.timestamp,
             mergeData.obj.size());
    }

    delete server;
    return 0;
}