//
// Created by lining on 8/8/23.
//
#include <string>
#include <algorithm>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "signalControl/signalControlCom.h"
#include "signalControl/SignalControl.h"
#include "common/CRC.h"


void ComTest() {
    //转移字符为0x5c 要转义的字节集为 0x7e 0x7d 0x5c
    vector<uint8_t> in = {0x7e, 0x12, 0x34, 0x56, 0x5c, 0x24, 0x7d};
    uint8_t tf = 0x5c;
    vector<uint8_t> needSet = {0x7e, 0x7d, 0x5c};

    auto out = TransferMean(in, tf, needSet);
    fmt::print("out:{::#x}\n", out);

    auto result = DeTransferMean(out, tf, needSet);
    fmt::print("result:{::#x}\n", result);
}

void ComFrame_GBT20999_2017Test() {

    // 7e 0022 0100 05 000c48fc 01 13 30 03
    // 01 05 11 01 02 01 01
    // 02 05 11 01 03 01 01
    // 03 05 11 01 04 01 21
    // e443 7d

    using namespace ComFrame_GBT20999_2017;
    FrameAll frame;
    frame.version = 0x0100;
    frame.controlCenterID = 0x05;
    frame.roadTrafficSignalControllerID = 0x000c48fc;
    frame.roadID = 0x01;
    frame.sequence = 0x13;
    frame.type = ComFrame_GBT20999_2017::Type_Set;
    frame.dataItemNum = 0x03;
    ComFrame_GBT20999_2017::DataItem dataItem;
    dataItem.index = 1;
    dataItem.length = 5;
    dataItem.typeID = 0x11;
    dataItem.objID = 0x01;
    dataItem.attrID = 0x02;
    dataItem.elementID = 0x01;
    dataItem.data.push_back(0x01);
    frame.dataItems.push_back(dataItem);
    ComFrame_GBT20999_2017::DataItem dataItem1;
    dataItem1.index = 2;
    dataItem1.length = 5;
    dataItem1.typeID = 0x11;
    dataItem1.objID = 0x01;
    dataItem1.attrID = 0x03;
    dataItem1.elementID = 0x01;
    dataItem1.data.push_back(0x01);
    frame.dataItems.push_back(dataItem1);
    ComFrame_GBT20999_2017::DataItem dataItem2;
    dataItem2.index = 3;
    dataItem2.length = 5;
    dataItem2.typeID = 0x11;
    dataItem2.objID = 0x01;
    dataItem2.attrID = 0x04;
    dataItem2.elementID = 0x01;
    dataItem2.data.push_back(0x21);
    frame.dataItems.push_back(dataItem2);

    vector<uint8_t> plainBytes;
    frame.setToBytes(plainBytes);
    fmt::print("plainBytes:{::#x}\n", plainBytes);

    vector<uint8_t> sendBytes;
    frame.setToBytesWithTF(sendBytes);


    FrameAll frameGet;
    frameGet.getFromBytesWithTF(sendBytes);

    vector<uint8_t> getPlainBytes;
    frameGet.setToBytes(getPlainBytes);

    fmt::print("getPlainBytes:{::#x}\n", getPlainBytes);
}

void crc16_test() {
    uint8_t plain[] = {0x00, 0x13, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x01, 0x12,
                       0x10, 0x01, 0x01, 0x04, 0x0c, 0x02, 0x03, 0x00};
    int len = sizeof(plain);
    uint16_t crc = Crc16Cal(plain, sizeof(plain), 0x1005, 0x0000, 0x0000, 0);
    printf("%x\n", crc);

}

bool isThreadRun = false;

static void RecvThread(int sock, struct sockaddr *local, socklen_t local_len) {
    uint8_t bufR[1024];
    int lenR = 0;
    memset(bufR, 0, 1024);
    LOG(INFO) << "thread recv begin";
    while (isThreadRun) {
        memset(bufR, 0, 1024);
        lenR = recvfrom(sock, bufR, 1024, 0, (struct sockaddr *) &local, &local_len);
        if (lenR > 0) {
//                    printf("recv ok\n");
            LOG(INFO) << "thread recv ok";
            vector<uint8_t> recvPlain;
            for (int i = 0; i < lenR; i++) {
                recvPlain.push_back(bufR[i]);
            }

            fmt::print("recvPlain:{::#x}\n", recvPlain);
        } else {
            printf("thread recv fail:%d\n", errno);
        }
    }
    LOG(INFO) << "thread recv end";
}



//10.100.24.28 15050 udp
DEFINE_string(cloudIp, "10.100.24.28", "ip，默认 10.100.24.28");
DEFINE_int32(cloudPort, 15050, "端口号，默认15050");
DEFINE_int32(localPort, 16000, "端口号，默认16000");

int main(int argc, char **argv) {
    std::cout << __cplusplus << std::endl;
    vector<string> unOrder= {"a","avddd","dada","dadas"};
    std::cout<<fmt::format("{}",unOrder)<< std::endl;

//    crc16_test();
//    ComTest();
//    ComFrame_GBT20999_2017Test();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;
    //开启udp客户端
    auto udpClient = new SignalControl(FLAGS_cloudIp, FLAGS_cloudPort, FLAGS_localPort);
    if (udpClient->Open() != 0) {
        LOG(ERROR) << "signal control open fail";
        delete udpClient;
        return -1;
    }

    printf("udp client sock:%d\n", udpClient->sock);

    string usage = "输入 1:发送查询\n"
                   "    2:发送设置\n"
                   "    3:发送扩展\n"
                   "    q:退出\n";
    printf("%s\n", usage.c_str());

    bool isExit = false;
    isThreadRun = true;
    auto local_len = sizeof(struct sockaddr);
//    std::thread th(RecvThread, udpClient->sock, (struct sockaddr *) &udpClient->local, local_len);
//    th.detach();

    auto addrLen = sizeof(struct sockaddr_in);
    while (!isExit) {
        string user;
        cout << "please enter(q:quit):" << endl;
        cin >> user;

        uint8_t bufR[1024];
        int lenR = 0;
        memset(bufR, 0, 1024);

        if (user == "q") {
            isExit = true;
        } else if (user == "1") {
            using namespace ComFrame_GBT20999_2017;
            FrameAll frame;
            frame.version = 0x0100;
            frame.controlCenterID = 0x05;
            frame.roadTrafficSignalControllerID = 21;
            frame.roadID = 0x00;
            frame.sequence = 0x12;
            frame.type = ComFrame_GBT20999_2017::Type_Query;
            frame.dataItemNum = 0x01;
            ComFrame_GBT20999_2017::DataItem dataItem;
            dataItem.index = 1;
            dataItem.length = 4;
            dataItem.typeID = 1;
            dataItem.objID = 2;
            dataItem.attrID = 0;
            dataItem.elementID = 0;
            frame.dataItems.push_back(dataItem);

            vector<uint8_t> sendBytes;
            frame.setToBytesWithTF(sendBytes);
            fmt::print("sendBytes:{::#x}\n", sendBytes);

            FrameAll frameRecv;
            if (udpClient->Communicate(frame, frameRecv) == 0) {
                vector<uint8_t> recvBytes;
                frameRecv.setToBytes(recvBytes);
                fmt::print("recvBytes:{::#x}\n", recvBytes);
            }
        } else if (user == "2") {
            //用组包工具来做
            using namespace ComFrame_GBT20999_2017;
            FrameAll frame;
            frame.version = 0x0100;
            frame.controlCenterID = 0x05;
            frame.roadTrafficSignalControllerID = 21;
            frame.roadID = 0x00;
            frame.sequence = 0x12;
            frame.type = ComFrame_GBT20999_2017::Type_Query;
            frame.dataItemNum = 0x01;
            ComFrame_GBT20999_2017::DataItem dataItem;
            dataItem.index = 1;
            dataItem.length = 4;
            dataItem.typeID = 3;
            dataItem.objID = 1;
            dataItem.attrID = 0;
            dataItem.elementID = 0;
            frame.dataItems.push_back(dataItem);

            vector<uint8_t> sendBytes;
            frame.setToBytesWithTF(sendBytes);
            fmt::print("sendBytes:{::#x}\n", sendBytes);

            FrameAll frameRecv;
            if (udpClient->Communicate(frame, frameRecv) == 0) {
                vector<uint8_t> recvBytes;
                frameRecv.setToBytes(recvBytes);
                fmt::print("recvBytes:{::#x}\n", recvBytes);
            }
        } else if (user == "3") {
            //用组包工具来做
            using namespace ComFrame_GBT20999_2017;
            FrameAll frame;
            frame.version = 0x0100;
            frame.controlCenterID = 0x05;
            frame.roadTrafficSignalControllerID = 21;
            frame.roadID = 0x00;
            frame.sequence = 0x12;
            frame.type = ComFrame_GBT20999_2017::Type_Set;
            frame.dataItemNum = 0x01;
            ComFrame_GBT20999_2017::DataItem dataItem;
            dataItem.index = 1;
            dataItem.length = 4;
            dataItem.typeID = 192;
            dataItem.objID = 2;
            dataItem.attrID = 1;
            dataItem.elementID = 0;
            dataItem.data.push_back(0);
            dataItem.data.push_back(7);

            dataItem.data.push_back(0);

            dataItem.data.push_back(0);
            dataItem.data.push_back(3);

            dataItem.data.push_back(1);

            dataItem.data.push_back(1);

            dataItem.data.push_back(1);
            frame.dataItems.push_back(dataItem);

            vector<uint8_t> sendBytes;
            frame.setToBytesWithTF(sendBytes);
            fmt::print("sendBytes:{::#x}\n", sendBytes);

            int lenS = sendto(udpClient->sock, sendBytes.data(), sendBytes.size(), 0,
                              (struct sockaddr *) &udpClient->remote_addr, addrLen);
            if (lenS != sendBytes.size()) {
                printf("send fail:%d\n", errno);
            } else {
//                printf("send ok\n");
                LOG(INFO) << "send ok";
                lenR = recvfrom(udpClient->sock, bufR, 1024, 0, (struct sockaddr *) &udpClient->remote_addr,
                                (socklen_t *) &addrLen);
                if (lenR > 0) {
                    LOG(INFO) << "recv ok";
                    vector<uint8_t> recvBytes;
                    fmt::print("recvBytes:{::#x}\n", recvBytes);

                    FrameAll frameGet;
                    if (frameGet.getFromBytesWithTF(recvBytes) == 0) {
                        vector<uint8_t> getPlainBytes;
                        getPlainBytes.clear();
                        frameGet.setToBytes(getPlainBytes);
                        fmt::print("getPlainBytes:{::#x}\n", getPlainBytes);
                    }
                } else {
                    printf("recv fail:%d\n", errno);
                }
            }
        }else if (user == "4") {
            //用组包工具来做
            using namespace ComFrame_GBT20999_2017;
            FrameAll frame;
            frame.version = 0x0100;
            frame.controlCenterID = 0x05;
            frame.roadTrafficSignalControllerID = 21;
            frame.roadID = 0x00;
            frame.sequence = 0x12;
            frame.type = ComFrame_GBT20999_2017::Type_Trap;
            frame.dataItemNum = 0x01;
            ComFrame_GBT20999_2017::DataItem dataItem;
            dataItem.index = 1;
            dataItem.length = 4;
            dataItem.typeID = 14;
            dataItem.objID = 2;
            dataItem.attrID = 2;
            dataItem.elementID = 0;
            dataItem.data.push_back(0);

            frame.dataItems.push_back(dataItem);

            vector<uint8_t> sendBytes;
            frame.setToBytesWithTF(sendBytes);
            fmt::print("sendBytes:{::#x}\n", sendBytes);

            int lenS = sendto(udpClient->sock, sendBytes.data(), sendBytes.size(), 0,
                              (struct sockaddr *) &udpClient->remote_addr, addrLen);
            if (lenS != sendBytes.size()) {
                printf("send fail:%d\n", errno);
            } else {
//                printf("send ok\n");
                LOG(INFO) << "send ok";
                lenR = recvfrom(udpClient->sock, bufR, 1024, 0, (struct sockaddr *) &udpClient->remote_addr,
                                (socklen_t *) &addrLen);
                if (lenR > 0) {
                    LOG(INFO) << "recv ok";
                    vector<uint8_t> recvBytes;
                    fmt::print("recvBytes:{::#x}\n", recvBytes);

                    FrameAll frameGet;
                    if (frameGet.getFromBytesWithTF(recvBytes) == 0) {
                        vector<uint8_t> getPlainBytes;
                        getPlainBytes.clear();
                        frameGet.setToBytes(getPlainBytes);
                        fmt::print("getPlainBytes:{::#x}\n", getPlainBytes);
                    }
                } else {
                    printf("recv fail:%d\n", errno);
                }
            }
        }
        sleep(1);
    }
    delete udpClient;
    isThreadRun = false;
    return 0;
}
