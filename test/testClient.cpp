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
#include <uuid/uuid.h>
#include "os/os.h"


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

        try {
            json::decode(content, out);
        } catch (std::exception &e) {
            LOG(ERROR) << e.what();
            return -1;
        }

        return 0;
    }
}

int getTrafficFlowGather(TrafficFlowGather &out) {
    uuid_t uuid;
    char uuid_str[37];
    memset(uuid_str, 0, 37);
    uuid_generate_time(uuid);
    uuid_unparse(uuid, uuid_str);
    out.oprNum = string(uuid_str);
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
    uint8_t buf[1024 * 1024 * 4];
    uint32_t buf_len = 1024 * 1024 * 4;
    memset(buf, 0, 1024 * 1024 * 4);
    buf_len = Pack(pkg, buf, buf_len);

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
    uint8_t buf[1024 * 1024 * 4];
    uint32_t buf_len = 1024 * 1024 * 4;
    memset(buf, 0, 1024 * 1024 * 4);
    buf_len = Pack(pkg, buf, buf_len);

//    auto ret = send(sock, buf, buf_len, 0);

    if (client->_s.isNull() || client->isNeedReconnect) {
        return;
    }

    try {
        auto ret = client->_s.sendBytes(buf, buf_len);

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
    uint8_t buf[1024 * 1024 * 4];
    uint32_t buf_len = 1024 * 1024 * 4;
    memset(buf, 0, 1024 * 1024 * 4);
    buf_len = Pack(pkg, buf, buf_len);

    try {

        auto ret = send(sock, buf, buf_len, 0);
        if (ret < 0) {
            std::cout << "发送失败" << msgType << std::endl;
        } else if (ret != buf_len) {
            std::cout << "发送失败" << msgType << std::endl;
        }
    } catch (...) {

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

int getCrossTrafficJamAlarm(CrossTrafficJamAlarm &out, string pic) {
    uuid_t uuid;
    char uuid_str[37];
    memset(uuid_str, 0, 37);
    uuid_generate_time(uuid);
    uuid_unparse(uuid, uuid_str);
    out.oprNum = string(uuid_str);
    out.crossID = "crossID";
    auto now = std::chrono::system_clock::now();
    uint64_t timestampNow = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

    out.timestamp = timestampNow;

    std::time_t t(out.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    out.alarmTime = ss.str();

    vector<uint8_t> data;
    if (os::GetVectorFromFile(data, pic) != 0) {
        return -1;
    }
    char *base64 = new char[1024 * 1024 * 4];
    unsigned int base64Len = 0;
    os::base64_encode(data.data(), data.size(), (unsigned char *) base64, &base64Len);
    out.imageData = string(base64);
    delete[]base64;

    return 0;
}

uint64_t snCrossTrafficJamAlarm = 0;

void Task_CrossTrafficJamAlarm(MyTcpClient *client, CrossTrafficJamAlarm crossTrafficJamAlarm) {

    pthread_mutex_lock(&mtx);
    string msgType = "TrafficFlowGather";

    auto timestamp = std::chrono::system_clock::now();
    uint64_t timestamp_u = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count();

    crossTrafficJamAlarm.timestamp = timestamp_u;

    std::time_t t(crossTrafficJamAlarm.timestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    crossTrafficJamAlarm.alarmTime = ss.str();


    snCrossTrafficJamAlarm++;
    Pkg pkg;
    crossTrafficJamAlarm.PkgWithoutCRC(snCrossTrafficJamAlarm, 1030033983, pkg);

    try {

        auto ret = client->SendBase(pkg);
        if (ret < 0) {
            std::cout << "发送失败" << msgType << std::endl;
        }
    } catch (...) {

    }
    auto now = std::chrono::system_clock::now();
    uint64_t timestampSend = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    std::cout << "发送" << msgType << ",发送时间:" << to_string(timestampSend) << ",帧内时间:"
              << to_string(crossTrafficJamAlarm.timestamp) << std::endl;
    auto cost = timestampSend - timestamp_u;
    if (cost > 80) {
        std::cout << msgType << "发送耗时" << cost << "ms" << std::endl;
    }
//    pthread_mutex_unlock(&mtx);

}

DEFINE_string(cloudIp, "127.0.0.1", "云端ip，默认127.0.0.1");
DEFINE_int32(cloudPort, 9001, "云端端口号，默认9001");
DEFINE_string(pic, "test.jpeg", "测试图片，默认test.jpeg");

int main(int argc, char **argv) {

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    MyTcpClient *client = new MyTcpClient(FLAGS_cloudIp, FLAGS_cloudPort);
    if (client->Open() == 0) {
    }
    client->Run();

//获取实时数据和统计数据
//    FusionData fusionData;
//    TrafficFlowGather trafficFlowGather;
//
//    if (getFusionData(fusionData) != 0) {
//        std::cout << "fusionData get fail" << std::endl;
//    }
//
//    if (getTrafficFlowGather(trafficFlowGather) != 0) {
//        std::cout << "trafficFlowGather get fail" << std::endl;
//    }
//
//    //开启两个定时任务
//    os::Timer timerFusionData;
//    os::Timer timerTrafficFlowGather;
//    timerFusionData.start(80, std::bind(Task_FusionData, sockfd, fusionData));
//    timerTrafficFlowGather.start(500, std::bind(Task_TrafficFlowGather, sockfd, trafficFlowGather));
    CrossTrafficJamAlarm crossTrafficJamAlarm;
    if (getCrossTrafficJamAlarm(crossTrafficJamAlarm, FLAGS_pic) != 0) {
        return -1;
    }

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
        usleep(1000 * 80);
//        Task_FusionData1(client,fusionData);
        Task_CrossTrafficJamAlarm(client, crossTrafficJamAlarm);
        if (client->isNeedReconnect) {
            client->Reconnect();
        }
    }

//    timerFusionData.stop();
//    timerTrafficFlowGather.stop();
//    close(sockfd);

    return 0;
}
