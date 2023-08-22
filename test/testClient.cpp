//
// Created by lining on 2/18/22.
//

#include <arpa/inet.h>
#include <linux/socket.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include "common/CRC.h"
#include "common/common.h"
#include <gflags/gflags.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "os/timeTask.hpp"
#include "myTcp/MyTcpClient.hpp"

using namespace std;
using namespace common;


int getFusionData(FusionData &out) {
    //查看测试文件是否存在
    ifstream ifs;
    ifs.open("fusiondata.json", ios::in);
    if (!ifs.is_open()) {
        return -1;
    } else {
        stringstream buffer;
        buffer << ifs.rdbuf();
        string content(buffer.str());
        ifs.close();
        Json::Reader reader;
        Json::Value in;
        if (!reader.parse(content, in, false)) {
            return -1;
        }
        out.JsonUnmarshal(in);

        return 0;
    }
}

int getTrafficFlowGather(TrafficFlowGather &out) {
    out.oprNum = random_uuid();
    out.crossID = "crossID";
    auto now = std::chrono::system_clock::now();
    uint64_t timestampNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

    out.timestamp = timestampNow;

    std::time_t t(out.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    out.recordDateTime = ss.str();
    //最大20
    int max = 20;
    for (int i = 0; i < max; i++) {
        OneFlowData item;
        item.flowDirection = i;
        item.inAverageSpeed = i * 0.1;
        item.inCars = i;
        item.outAverageSpeed = i * 0.2;
        item.outCars = i;
        item.laneCode = to_string(i);
        item.laneDirection = i;
        item.queueCars = i;
        item.queueLen = i * 8;
        out.trafficFlow.push_back(item);
    }
    return 0;
}

pthread_mutex_t mtx;

uint64_t snFusionData = 0;

void Task_FusionData(int sock, FusionData fusionData) {

//    pthread_mutex_lock(&mtx);
    string msgType = "FusionData";

    auto timestamp = std::chrono::system_clock::now();
    uint64_t timestamp_u = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    fusionData.timestamp = timestamp_u;
    snFusionData++;
    Pkg pkg;
    fusionData.PkgWithoutCRC(snFusionData, 1030033983, pkg);
    uint8_t buf[1024 * 1024];
    uint32_t buf_len = 0;
    memset(buf, 0, 1024 * 1024);
    Pack(pkg, buf, &buf_len);

    auto ret = send(sock, buf, buf_len, 0);
    if (ret < 0) {
        std::cout << "发送失败" << msgType << std::endl;
    } else if (ret != buf_len) {
        std::cout << "发送失败" << msgType << std::endl;
    }

    auto now = std::chrono::system_clock::now();
    uint64_t timestampSend = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    std::cout << "发送" << msgType << ",发送时间:" << to_string(timestampSend) << ",帧内时间:"
              << to_string(fusionData.timestamp) << " " << buf_len << std::endl;
    auto cost = timestampSend - timestamp_u;
    if (cost > 80) {
        std::cout << msgType << "发送耗时" << cost << "ms" << std::endl;
    }
//    pthread_mutex_unlock(&mtx);

}

static void Task_FusionData1(MyTcpClient *client, FusionData fusionData) {

//    pthread_mutex_lock(&mtx);
    string msgType = "FusionData";

    auto timestamp = std::chrono::system_clock::now();
    uint64_t timestamp_u = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    fusionData.timestamp = timestamp_u;
    snFusionData++;
    Pkg pkg;
    fusionData.PkgWithoutCRC(snFusionData, 1030033983, pkg);
    uint8_t buf[1024 * 1024];
    uint32_t buf_len = 0;
    memset(buf, 0, 1024 * 1024);
    Pack(pkg, buf, &buf_len);

//    auto ret = send(sock, buf, buf_len, 0);

    if (client->_socket.isNull() || client->isNeedReconnect){
        return;
    }

    try {
        auto ret = client->_socket.sendBytes(buf, buf_len);

        if (ret < 0) {
            std::cout << "发送失败" << msgType << std::endl;
            return;
        } else if (ret != buf_len) {
            std::cout << "发送失败" << msgType << std::endl;
            return;
        }
    } catch (...) {
        std::cout << "发送失败1" << msgType << std::endl;
        client->isNeedReconnect = true;
        return;
    }
    auto now = std::chrono::system_clock::now();
    uint64_t timestampSend = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    std::cout << "发送" << msgType << ",发送时间:" << to_string(timestampSend) << ",帧内时间:"
              << to_string(fusionData.timestamp) << " " << buf_len << std::endl;
    auto cost = timestampSend - timestamp_u;
    if (cost > 80) {
        std::cout << msgType << "发送耗时" << cost << "ms" << std::endl;
    }
//    pthread_mutex_unlock(&mtx);

}



uint64_t snTrafficFlowGather = 0;

void Task_TrafficFlowGather(int sock, TrafficFlowGather trafficFlowGather) {

    pthread_mutex_lock(&mtx);
    string msgType = "TrafficFlowGather";

    auto timestamp = std::chrono::system_clock::now();
    uint64_t timestamp_u = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    trafficFlowGather.timestamp = timestamp_u;

    std::time_t t(trafficFlowGather.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    trafficFlowGather.recordDateTime = ss.str();


    snTrafficFlowGather++;
    Pkg pkg;
    trafficFlowGather.PkgWithoutCRC(snTrafficFlowGather, 1030033983, pkg);
    uint8_t buf[1024 * 1024];
    uint32_t buf_len = 0;
    memset(buf, 0, 1024 * 1024);
    Pack(pkg, buf, &buf_len);

    try {

        auto ret = send(sock, buf, buf_len, 0);
        if (ret < 0) {
            std::cout << "发送失败" << msgType << std::endl;
        } else if (ret != buf_len) {
            std::cout << "发送失败" << msgType << std::endl;
        }
    }catch (...){

    }
    auto now = std::chrono::system_clock::now();
    uint64_t timestampSend = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    std::cout << "发送" << msgType << ",发送时间:" << to_string(timestampSend) << ",帧内时间:"
              << to_string(trafficFlowGather.timestamp) << " " << buf_len << std::endl;
    auto cost = timestampSend - timestamp_u;
    if (cost > 80) {
        std::cout << msgType << "发送耗时" << cost << "ms" << std::endl;
    }
//    pthread_mutex_unlock(&mtx);

}


int signalIgnPipe() {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) < 0) {
        printf("call sigaction fail, %s\n", strerror(errno));
        return errno;
    }

    return 0;
}

DEFINE_string(cloudIp, "10.110.60.122", "云端ip，默认 10.110.60.122");
DEFINE_int32(cloudPort, 9988, "云端端口号，默认9988");

int main(int argc, char **argv) {
    signalIgnPipe();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
//    //初始化一个client
//    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//
//    if (sockfd <= 0) {
//        printf("client <=0\n");
//        return -1;
//    }
//
//    int opt = 1;
//    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
////    timeval timeout;
////    timeout.tv_sec = 3;
////    timeout.tv_usec = 0;
////    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
////    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
////
////    int recvSize = 0;
////    int sendSize = 0;
////    socklen_t optlen = sizeof(int);
////
////    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvSize, &optlen);
////    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendSize, &optlen);
////
////    printf("原始缓存大小，接收%d 发送%d\n", recvSize, sendSize);
////
////
////    int recvBufSize = 256 * 1024;
////    int sendBufSize = 256 * 1024;
////    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
////    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));
////
////    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvSize, &optlen);
////    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendSize, &optlen);
////
////    printf("设置后缓存大小，接收%d 发送%d\n", recvSize, sendSize);
////
////    string server_ip = "127.0.0.1";
////    uint16_t server_port = 9000;
//
//    string server_ip = FLAGS_cloudIp;
//    uint16_t server_port = FLAGS_cloudPort;
//
//    struct sockaddr_in server_addr;
//    int ret = 0;
//    memset(&server_addr, 0, sizeof(server_addr));
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_port = htons(server_port);
//
//    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
//    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
//
//    if (ret == -1) {
//        printf("connect server:%s-%d fail\n", server_ip.c_str(), server_port);
//        close(sockfd);
//        return -1;
//    }
//    printf("connect server:%s-%d success\n", server_ip.c_str(), server_port);


//    //获取指定目录下的文件列表
//    int roadNum = 0;
//    string path;
//    GetData *getData = nullptr;
//    if (argc > 3) {
//        roadNum = atoi(argv[1]);
//        path = string(argv[2]);
//        Info("road:%d,path:%s", roadNum, path.c_str());
//        getData = new GetData(path);
//        getData->GetOrderListFileName(path);
//    }

    MyTcpClient *client = new MyTcpClient(FLAGS_cloudIp, FLAGS_cloudPort);
    if (client->Open() == 0) {
        client->Run();
    }

//获取实时数据和统计数据
    FusionData fusionData;
    TrafficFlowGather trafficFlowGather;

    if (getFusionData(fusionData) != 0) {
        std::cout << "fusionData get fail" << std::endl;
    }

    if (getTrafficFlowGather(trafficFlowGather) != 0) {
        std::cout << "trafficFlowGather get fail" << std::endl;
    }

    //开启两个定时任务
    os::Timer timerFusionData;
    os::Timer timerTrafficFlowGather;
//    timerFusionData.start(80, std::bind(Task_FusionData, sockfd, fusionData));
//    timerTrafficFlowGather.start(500, std::bind(Task_TrafficFlowGather, sockfd, trafficFlowGather));

    bool isExit = false;

    while (!isExit) {
//        string user;
//
//        cout << "please enter(q:quit):" << endl;
//        cin >> user;
//
//        if (user == "q") {
//            isExit = true;
//            continue;
//        }
        usleep(1000*80);
        Task_FusionData1(client,fusionData);
        if (client->isNeedReconnect){
            client->Reconnect();
        }
    }

    timerFusionData.stop();
//    timerTrafficFlowGather.stop();
//    close(sockfd);

    return 0;
}
