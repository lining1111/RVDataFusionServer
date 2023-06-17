//
// Created by lining on 2023/6/15.
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
#include "os/os.h"

using namespace std;
using namespace common;
using namespace os;

void readTXT(std::string txt_path_, std::vector<FusionData> &fusionDatas) {
    ifstream infile;
    infile.open(txt_path_);
    if (!infile) {
        cout << "无法打开文件！" << endl;
        exit(1);
    }
    //1.将文件的内容按行分别读入行集合
    vector<string> lines;
    string line;
    line.clear();
    while (getline(infile, line)) {
        //如果该行不为空
        if (!line.empty()) {
            //去掉\r\n
            while (line[line.size() - 1] == '\n' || line[line.size() - 1] == '\r') {
                line.erase(line.end() - 1);
            }
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
    }
    infile.close();
    //2.将每行以空格分割成字符串集
    string delimiter = " ";
    vector<vector<string>> contents;
    for (auto lineOne: lines) {
        vector<string> content;
        content.clear();
        content = split(lineOne, delimiter);
        contents.push_back(content);
    }
    //3.逐行分析，如果是以frame开头的，且下一行不是以frame开始的，就为一帧。
    vector<int> frameIndex;
    string StartFrame = "frame";
    for (int i = 0; i < contents.size() - 1; i++) {
        auto iterS = contents.at(i);
        auto iterE = contents.at(i + 1);
        if (iterS.at(0) == StartFrame && iterE.at(0) != StartFrame) {
            frameIndex.push_back(i);
        }
    }
    //4.找到每帧的开始和结束
    typedef struct {
        int start;//开始的行
        int end; //结束的行
    } OneFramePos;
    vector<OneFramePos> frames;
    frameIndex.push_back(contents.size());
    for (int i = 0; i < frameIndex.size() - 1; i++) {
        auto iterS = frameIndex.at(i);
        auto iterE = frameIndex.at(i + 1);

        OneFramePos item;
        item.start = iterS + 1;
        item.end = iterE - 1;
        frames.push_back(item);
    }
    //5.根据每帧的记录开始和结束行来读取信息
    enum {
        ElementIndex_showID = 0,
        ElementIndex_left = 1,
        ElementIndex_top = 2,
        ElementIndex_width = 3,
        ElementIndex_height = 4,
        ElementIndex_cameraID = 5,
        ElementIndex_detectID = 6,
        ElementIndex_longitude = 7,
        ElementIndex_latitude = 8,
        ElementIndex_directionAngle = 9,
        ElementIndex_speed = 10,
    } ElementIndex;
    for (auto iter: frames) {
        FusionData item;
        item.oprNum = random_uuid();
        item.crossID = "crossID";
        item.isHasImage = false;
        for (int i = iter.start; i < iter.end + 1; i++) {
            auto lineContent = contents.at(i);
            printf("%d\n", i);
            ObjMix objMix;
            objMix.objID = lineContent.at(ElementIndex_showID);
            objMix.angle = atof(lineContent.at(ElementIndex_directionAngle).c_str());
            objMix.speed = atof(lineContent.at(ElementIndex_speed).c_str());
            objMix.longitude = atof(lineContent.at(ElementIndex_longitude).c_str());
            objMix.latitude = atof(lineContent.at(ElementIndex_latitude).c_str());

            item.lstObjTarget.push_back(objMix);
        }
        fusionDatas.push_back(item);
    }
}

//发送数据到服务器
int sn = 0;

int SendServer(int sock, FusionData fusionData, string matrixNo) {
    uint8_t buff[1024 * 1024];
    uint32_t len = 0;
    uint32_t deviceNo = stoi(matrixNo.substr(0, 10));
    Pkg pkg;
    fusionData.PkgWithoutCRC(sn, deviceNo, pkg);
    Pack(pkg, buff, &len);
    int ret = send(sock, buff, len, 0);
    if (ret == -1) {
        printf("发生失败 errno:%d\n", errno);
        return -2;
    } else if (ret != len) {
        return -1;
    } else {
        return 0;
    }

}


DEFINE_string(cloudIp, "10.110.60.122", "云端ip，默认 10.110.60.122");
DEFINE_int32(cloudPort, 9988, "云端端口号，默认9988");
DEFINE_string(file, "data.txt", "数据文件名，包括路径，默认 data.txt");
DEFINE_string(matrixNo, "123456789", "matrixNo，默认 123456789");
DEFINE_string(crossID, "crossID", "路口号，默认 crossID");
DEFINE_int32(fs, 80, "数据帧率，默认80,单位ms");


int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    //从文件中获取全部帧数据
    std::vector<FusionData> fusionDatas;
    readTXT(FLAGS_file, fusionDatas);

    //连接云端TCP
    //初始化一个client
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd <= 0) {
        printf("client <=0\n");
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
//    timeval timeout;
//    timeout.tv_sec = 3;
//    timeout.tv_usec = 0;
//    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
//
//    int recvSize = 0;
//    int sendSize = 0;
//    socklen_t optlen = sizeof(int);
//
//    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvSize, &optlen);
//    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendSize, &optlen);
//
//    printf("原始缓存大小，接收%d 发送%d\n", recvSize, sendSize);
//
//
//    int recvBufSize = 256 * 1024;
//    int sendBufSize = 256 * 1024;
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvBufSize, sizeof(int));
//    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendBufSize, sizeof(int));
//
//    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *) &recvSize, &optlen);
//    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendSize, &optlen);
//
//    printf("设置后缓存大小，接收%d 发送%d\n", recvSize, sendSize);
//
//    string server_ip = "127.0.0.1";
//    uint16_t server_port = 9000;

    string server_ip = FLAGS_cloudIp;
    uint16_t server_port = FLAGS_cloudPort;

    struct sockaddr_in server_addr;
    int ret = 0;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    ret = connect(sockfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    if (ret == -1) {
        printf("connect server:%s-%d fail\n", server_ip.c_str(), server_port);
        close(sockfd);
        return -1;
    }
    printf("connect server:%s-%d success\n", server_ip.c_str(), server_port);

    //开启发送
    auto fs = FLAGS_fs;
    auto matrixNo = FLAGS_matrixNo;
    for (int i = 0; i < fusionDatas.size(); i++) {
        usleep(1000 * fs);
        auto iter = fusionDatas.at(i);
        printf("开始发送第%i帧\n", i);
        uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        iter.timstamp = timestampStart;
        iter.crossID = FLAGS_crossID;
        int ret = SendServer(sockfd, iter, matrixNo);
        uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        string result;
        if (ret == 0) {
            result = "成功";
        } else {
            result = "失败";
        }
        cout << "发送" << result << server_ip << ":" << server_port
             << ",发送开始时间:" << to_string(timestampStart)
             << ",发送结束时间:" << to_string(timestampEnd)
             << ",帧内时间:" << to_string((uint64_t) iter.timstamp)
             << ",耗时" << (timestampEnd - timestampStart) << "ms";
    }

    close(sockfd);
    return 0;
}