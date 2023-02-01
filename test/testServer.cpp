//
// Created by lining on 2/18/22.
//

//#include <unistd.h>
//#include <arpa/inet.h>
//#include <iostream>
//#include "server/FusionServer.h"
//#include "log/Log.h"
//#include <sys/socket.h>


//using namespace z_log;
//
//int main(int argc, char **argv) {
//
//    FusionServer *server = new FusionServer();
//
//    server->Open();
//    server->Run();
//
//    while (server->isRun.load()) {
//        usleep(1);
//        //循环出读server各个client内的WatchData队列
////        if (server->vectorClient.empty()) {
////            Info("客户端队列为空");
////            continue;
////        }
////        cout << "客户端数量：" << to_string(server->vectorClient.size()) << endl;
//
////        //读取客户端的queueWatchData
////        pthread_mutex_lock(&server->lockVectorClient);
////        if (server->vectorClient.empty()) {
////            pthread_cond_wait(&server->condVectorClient, &server->lockVectorClient);
////        }
////        for (auto iter: server->vectorClient) {
////            if (iter->queueWatchData.size() == 0) {
////                Info("此客户端%d-%s中 watchData为空", iter->sock, inet_ntoa(iter->clientAddr.sin_addr));
////            } else {
////                //取出所有消息，并打印部分信息
////                do {
////                    WatchData watchData;
////                    pthread_mutex_lock(&iter->lockWatchData);
////                    if (iter->queueWatchData.empty()) {
////                        pthread_cond_wait(&iter->condWatchData, &iter->lockWatchData);
////                    }
////
////                    watchData = iter->queueWatchData.front();
////                    iter->queueWatchData.pop();
////
////                    Info("WatchData:%s-%f ip:%s", watchData.oprNum.c_str(), watchData.RecordDateTime,
////                         watchData.cameraIp.c_str());
////                    pthread_cond_broadcast(&iter->condWatchData);
////                    pthread_mutex_unlock(&iter->lockWatchData);
////
////                } while (iter->queueWatchData.size() > 0);
////
////            }
////        }
////
////        pthread_mutex_unlock(&server->lockVectorClient);
//        //读取服务端的queueMergeData
//        if (server->queueMergeData.size() == 0) {
////            Info("server 融合数据队列为空");
//            continue;
//        }
//        FusionServer::MergeData mergeData = server->queueMergeData.front();
//        server->queueMergeData.pop();
//        Info("server mergeData: timestamp:%f,size:%d",
//             mergeData.timestamp,
//             mergeData.objOutput.size());
//    }
//
//    delete server;
//
//
//
//    return 0;
//}

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#define SERVER_PORT 9001
#define MAX_SERVERS 10

int main(int argc, char **argv) {
    int fd_socket = 0;
    int is_bind = 0;
    int is_listen = 0;
    int fd_client = 0;
    int client_num = 0;
    socklen_t add_len = 0;
    unsigned int recv_len = 0;
    struct sockaddr_in sock_server_addr;
    socklen_t sock_len;
    unsigned char recv_buffer[1024*1024*2] = {0};
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket == -1) {
        return fd_socket;
    }

    sock_server_addr.sin_family = AF_INET;
    sock_server_addr.sin_port = htons(SERVER_PORT);
    sock_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(sock_server_addr.sin_zero, 0, 8);

    is_bind = bind(fd_socket, (const struct sockaddr *) &sock_server_addr, \
                    sizeof(struct sockaddr));
    if (is_bind == -1) {
        return is_bind;
    }

    is_listen = listen(fd_socket, MAX_SERVERS);
    if (is_listen == -1) {
        return -1;
    }

    while (1) {
        add_len = sizeof(struct sockaddr);
        fd_client = accept(fd_socket, (struct sockaddr *) &sock_server_addr, &add_len);
        if (fd_client != -1) {
            client_num++;
            printf("Get connection from client %d: %s\n", client_num, inet_ntoa(sock_server_addr.sin_addr));
            if (!fork()) {
                while (1) {
                    recv_len = recv(fd_client, recv_buffer, sizeof(recv_buffer) - 1, 0);
                    if (recv_len <= 0) {
                        close(fd_socket);
                        return -1;
                    } else {
//                        recv_buffer[recv_len] = '\0';
                        printf("Got message from client %d: len:%d\n", client_num, recv_len);
                    }
                }
            }
        }

    }
    close(fd_socket);

    return 0;
}