//
// Created by lining on 2/18/22.
//

#include <unistd.h>
#include <arpa/inet.h>
#include "server/FusionServer.h"
#include "log/Log.h"
#include "merge/merge.h"

using namespace log;

int main(int argc, char **argv) {

    FusionServer *server = new FusionServer();

    server->Open();
    server->Run();

    while (server->isRun) {
        sleep(1);
        //循环出读server各个client内的WatchData队列
        if (server->vector_client.empty()) {
            Info("客户端队列为空");
            continue;
        }

        pthread_mutex_lock(&server->lock_vector_client);
        if (server->vector_client.empty()) {
            pthread_cond_wait(&server->cond_vector_client, &server->lock_vector_client);
        }
        for (auto iter: server->vector_client) {
            if (iter->queueWatchData.Size() == 0) {
                Info("此客户端%d-%s中 watchData为空", iter->sock, inet_ntoa(iter->clientAddr.sin_addr));
            } else {
                //取出所有消息，并打印部分信息
                do {
                    WatchData watchData;
                    watchData = iter->queueWatchData.PopFront();
                    Info("WatchData:%s-%f ip:%s", watchData.oprNum.c_str(), watchData.RecordDateTime,
                         watchData.cameraIp.c_str());
                } while (iter->queueWatchData.Size() > 0);

            }
        }

        pthread_mutex_unlock(&server->lock_vector_client);
    }

    delete server;
    return 0;
}