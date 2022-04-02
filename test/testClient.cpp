//
// Created by lining on 2/18/22.
//

#include <arpa/inet.h>
#include <linux/socket.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <common/CRC.h>
#include <GetData.h>
#include "log/Log.h"
#include "common/common.h"

using namespace log;
using namespace std;
using namespace common;


int Msg1(uint8_t *out, uint32_t *len) {
    //set struct
    WatchData watchData;

    timeval tv;
    gettimeofday(&tv, nullptr);

    watchData.oprNum = random_uuid().data();
    watchData.hardCode = "hardCode";
    watchData.timstamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    watchData.matrixNo = "matrixNo";
    watchData.cameraIp = "192.168.1.100";
    watchData.RecordDateTime = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    watchData.isHasImage = 1;
    uint8_t img[4] = {1, 2, 3, 4};

    char imgBase64[16];
    unsigned int imgBase64Len = 0;

    base64_encode((unsigned char *) img, 4, (unsigned char *) imgBase64, &imgBase64Len);

    watchData.imageData = string(imgBase64).data();
    watchData.direction = East;

    //AnnuciatorInfo

    AnnuciatorInfo annuciatorInfo1;
    annuciatorInfo1.LightID = 1;
    annuciatorInfo1.Light = "R";
    annuciatorInfo1.RT = 123;

    AnnuciatorInfo annuciatorInfo2;
    annuciatorInfo2.LightID = 2;
    annuciatorInfo2.Light = "G";
    annuciatorInfo2.RT = 456;

    AnnuciatorInfo annuciatorInfo3;
    annuciatorInfo3.LightID = 3;
    annuciatorInfo3.Light = "Y";
    annuciatorInfo3.RT = 789;
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo1);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo2);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo3);

    //lstObjTarget

    ObjTarget objTarget1;
    objTarget1.objID = 1;
    objTarget1.objType = 1;
    objTarget1.plates = "冀A123456";
    objTarget1.plateColor = "BLUE";
    objTarget1.left = 1;
    objTarget1.top = 2;
    objTarget1.right = 3;
    objTarget1.bottom = 4;
    objTarget1.locationX = 5;
    objTarget1.locationY = 6;
    objTarget1.distance = "很近";
    objTarget1.directionAngle = -45;
    objTarget1.speed = "很快";

    ObjTarget objTarget2;
    objTarget2.objID = 2;
    objTarget2.objType = 2;
    objTarget2.plates = "冀A234567";
    objTarget2.plateColor = "BLUE";
    objTarget2.left = 1;
    objTarget2.top = 2;
    objTarget2.right = 3;
    objTarget2.bottom = 4;
    objTarget2.locationX = 5;
    objTarget2.locationY = 6;
    objTarget2.distance = "很近";
    objTarget2.directionAngle = -45;
    objTarget2.speed = "很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    string jsonMarshal;

    JsonMarshalWatchData(watchData, jsonMarshal);

    Pkg pkg;
    int pkg_len = 0;
    //1.头部
    pkg.head.tag = '$';
    pkg.head.version = 1;
    pkg.head.cmd = CmdType::DeviceData;
    pkg.head.sn = 1;
    pkg.head.deviceNO = 0x12345678;
    pkg.head.len = 0;
    pkg_len += sizeof(pkg.head);
    //2.正文string
    pkg.body = jsonMarshal;
    pkg_len += jsonMarshal.length();
    //3检验值
    pkg.crc.data = 0x0000;
    pkg_len += sizeof(pkg.crc);
    //4 长度信息
    pkg.head.len = pkg_len;

    //组包
    Pack(pkg, out, len);
}

int Msg2(uint8_t *out, uint32_t *len) {
    //set struct
    WatchData watchData;

    timeval tv;
    gettimeofday(&tv, nullptr);

    watchData.oprNum = random_uuid().data();
    watchData.hardCode = "hardCode";
    watchData.timstamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    watchData.matrixNo = "matrixNo";
    watchData.cameraIp = "192.168.1.101";
    watchData.RecordDateTime = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
    watchData.isHasImage = 1;
    uint8_t img[4] = {1, 2, 3, 4};

    char imgBase64[16];
    unsigned int imgBase64Len = 0;

    base64_encode((unsigned char *) img, 4, (unsigned char *) imgBase64, &imgBase64Len);

    watchData.imageData = string(imgBase64).data();
    watchData.direction = West;

    //AnnuciatorInfo

    AnnuciatorInfo annuciatorInfo1;
    annuciatorInfo1.LightID = 1;
    annuciatorInfo1.Light = "R";
    annuciatorInfo1.RT = 123;

    AnnuciatorInfo annuciatorInfo2;
    annuciatorInfo2.LightID = 2;
    annuciatorInfo2.Light = "G";
    annuciatorInfo2.RT = 456;

    AnnuciatorInfo annuciatorInfo3;
    annuciatorInfo3.LightID = 3;
    annuciatorInfo3.Light = "Y";
    annuciatorInfo3.RT = 789;
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo1);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo2);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo3);

    //lstObjTarget

    ObjTarget objTarget1;
    objTarget1.objID = 1;
    objTarget1.objType = 1;
    objTarget1.plates = "冀B123456";
    objTarget1.plateColor = "BLUE";
    objTarget1.left = 1;
    objTarget1.top = 2;
    objTarget1.right = 3;
    objTarget1.bottom = 4;
    objTarget1.locationX = 5;
    objTarget1.locationY = 6;
    objTarget1.distance = "很近";
    objTarget1.directionAngle = -45;
    objTarget1.speed = "很快";

    ObjTarget objTarget2;
    objTarget2.objID = 2;
    objTarget2.objType = 2;
    objTarget2.plates = "冀B234567";
    objTarget2.plateColor = "BLUE";
    objTarget2.left = 1;
    objTarget2.top = 2;
    objTarget2.right = 3;
    objTarget2.bottom = 4;
    objTarget2.locationX = 5;
    objTarget2.locationY = 6;
    objTarget2.distance = "很近";
    objTarget2.directionAngle = -45;
    objTarget2.speed = "很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    string jsonMarshal;

    JsonMarshalWatchData(watchData, jsonMarshal);

    Pkg pkg;
    int pkg_len = 0;
    //1.头部
    pkg.head.tag = '$';
    pkg.head.version = 1;
    pkg.head.cmd = CmdType::DeviceData;
    pkg.head.sn = 2;
    pkg.head.deviceNO = 0x87654321;
    pkg.head.len = 0;
    pkg_len += sizeof(pkg.head);
    //2.正文string
    pkg.body = jsonMarshal;
    pkg_len += jsonMarshal.length();
    //3检验值
    pkg.crc.data = 0x0000;
    pkg_len += sizeof(pkg.crc);
    //4 长度信息
    pkg.head.len = pkg_len;

    //组包
    Pack(pkg, out, len);
}


int main(int argc, char **argv) {

    //初始化一个client
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd <= 0) {
        Fatal("client <=0");
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));

    int recvBufSize = 32 * 1024;
    int sendBufSize = 32 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));

    string server_ip = "127.0.0.1";
    uint16_t server_port = 10000;

    struct sockaddr_in server_addr;
    int ret = 0;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);//端口5000

    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    if (ret == -1) {
        Error("connect server:%s-%d fail", server_ip.c_str(), server_port);
        close(sockfd);
        return -1;
    }
    Info("connect server:%s-%d success", server_ip.c_str(), server_port);


    //获取指定目录下的文件列表
    int roadNum = atoi(argv[1]);
    string path = string(argv[2]);
    Info("road:%d,path:%s", roadNum, path.c_str());
    GetData *getData = new GetData(path);
    getData->GetOrderListFileName(path);


    bool isExit = false;

    while (!isExit) {
        string user;

        cout << "please enter(q:quit):" << endl;
        cin >> user;

        if (user == "q") {
            isExit = true;
            continue;
        }

        uint8_t msg[1024 * 1024];
        uint32_t msg_len = 0;

        bzero(msg, ARRAY_SIZE(msg));
        if (user == "1") {
            //send WatchData 1
            Msg1(msg, &msg_len);
            int len = send(sockfd, msg, msg_len, 0);
            //打印下buffer
//            PrintHex(msg, msg_len);

            uint16_t crc = 0;
            memcpy(&crc, msg + msg_len - 2, 2);
            Info("crc send:%d", crc);

            if (len != msg_len) {
                Error("send fail");
            } else {
                Info("send success len:%d", len);
            }

        } else if (user == "2") {
            //send WatchData 2
            Msg2(msg, &msg_len);
            int len = send(sockfd, msg, msg_len, 0);
            //打印下buffer
//            PrintHex(msg, msg_len);

            if (len != msg_len) {
                Error("send fail");
            } else {
                Info("send success len:%d", len);
            }
        }
        int index = 0;
        if (user == "send") {
            //依次发送
            string file;
            if ((index + 1) < getData->files.size()) {
                file = getData->files.at(index);
            } else {
                index = 0;
                file = getData->files.at(index);
            }
            getData->GetDataFromOneFile(getData->path + "/" + file);
            getData->GetObjFromData(getData->data);
            WatchData watchData;
            timeval tv;
            gettimeofday(&tv, nullptr);

            watchData.oprNum = random_uuid().data();
            watchData.hardCode = "hardCode";
            watchData.timstamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
            watchData.matrixNo = "matrixNo";
            watchData.cameraIp = "192.168.1.100";
            watchData.RecordDateTime = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
            watchData.isHasImage = 1;
            uint8_t img[4] = {1, 2, 3, 4};

            char imgBase64[16];
            unsigned int imgBase64Len = 0;

            base64_encode((unsigned char *) img, 4, (unsigned char *) imgBase64, &imgBase64Len);

            watchData.imageData = string(imgBase64).data();
            watchData.direction = roadNum;
            //lstObjTarget
            for (int i = 0; i < getData->obj.size(); i++) {
                auto iter = getData->obj.at(i);
                watchData.lstObjTarget.push_back(iter);
            }
            //组包
            Pkg pkg;
            PkgWatchDataWithoutCRC(watchData, index, 0x12345678, pkg);
            index++;
            Pack(pkg, msg, &msg_len);
            int len = send(sockfd, msg, msg_len, 0);
            //打印下buffer
//            PrintHex(msg, msg_len);

            if (len != msg_len) {
                Error("send fail");
            } else {
                Info("send success len:%d", len);
            }

        } else if (user == "sendall") {
            //发送全部
            //1.读取路径下所有文件
            //依次发送
            for (int i = 0; i < getData->files.size(); i++) {
                string file = getData->files.at(i);
                getData->GetDataFromOneFile(getData->path + "/" + file);
                getData->GetObjFromData(getData->data);
                WatchData watchData;
                timeval tv;
                gettimeofday(&tv, nullptr);

                watchData.oprNum = random_uuid().data();
                watchData.hardCode = "hardCode";
                watchData.timstamp = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
                watchData.matrixNo = "matrixNo";
                watchData.cameraIp = "192.168.1.100";
                watchData.RecordDateTime = (tv.tv_sec * 1000 + tv.tv_usec / 1000);
                watchData.isHasImage = 1;
                uint8_t img[4] = {1, 2, 3, 4};

                char imgBase64[16];
                unsigned int imgBase64Len = 0;

                base64_encode((unsigned char *) img, 4, (unsigned char *) imgBase64, &imgBase64Len);

                watchData.imageData = string(imgBase64).data();
                watchData.direction = roadNum;
                //lstObjTarget
                for (int j = 0; j < getData->obj.size(); j++) {
                    auto iter = getData->obj.at(j);
                    watchData.lstObjTarget.push_back(iter);
                }
                //组包
                Pkg pkg;
                PkgWatchDataWithoutCRC(watchData, index, 0x12345678, pkg);
                index++;
                Pack(pkg, msg, &msg_len);
                int len = send(sockfd, msg, msg_len, 0);
                //打印下buffer
//            PrintHex(msg, msg_len);

                if (len != msg_len) {
                    Error("send fail");
                } else {
                    Info("send success len:%d", len);
                }
                usleep(50 * 1000);
            }
        }

    }

    close(sockfd);

    return 0;
}
