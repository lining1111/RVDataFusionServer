//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <queue>
#include <sys/time.h>
#include <iostream>
#include "common/common.h"
#include "common/CRC.h"
#include "Queue.h"
#include "log/Log.h"
#include "ringBuffer/RingBuffer.h"

using namespace common;
using namespace z_log;

/**
 * 一次帧数据组包解包过程
 */
void examplePkg() {
    Pkg pkg;
    uint8_t buf[1024 * 512];
    int len = 0;
    bzero(buf, ARRAY_SIZE(buf));
    //1.头部
    pkg.head.tag = '$';
    pkg.head.version = 1;
    pkg.head.cmd = CmdType::DeviceData;
    pkg.head.sn = 1;
    pkg.head.deviceNO = 0x12345678;
    pkg.head.len = 0;
    memcpy(buf + len, &pkg.head, sizeof(pkg.head));
    len += sizeof(pkg.head);
    //2.正文
    //2.1方法名
    //2.2方法参数
    string body = "this is a test";
    pkg.body = body;
    memcpy(buf + len, pkg.body.data(), pkg.body.length());
    len += body.length();
    //3检验值
    pkg.crc.data = Crc16TabCCITT(buf, len);
    len += sizeof(pkg.crc);
    //4 长度信息
    pkg.head.len = len;

    //组包
    uint8_t dataEncode[1024 * 1024];
    uint32_t dataEncodeLen = 0;
    bzero(dataEncode, sizeof(dataEncode) / sizeof(dataEncode[0]));
    Pack(pkg, dataEncode, &dataEncodeLen);
    //解包
    Pkg frameDecode;
    Unpack(dataEncode, dataEncodeLen, frameDecode);
}

/**
 * 一次监控数据组包解包测试
 */
void exampleJsonWatchData() {
    //set struct
    WatchData watchData;

    timeval tv;
    gettimeofday(&tv, nullptr);

    watchData.oprNum = random_uuid().data();
    watchData.hardCode = "hardCode";
    watchData.timstamp = tv.tv_sec;
    watchData.matrixNo = "matrixNo";
    watchData.cameraIp = "192.168.1.100";
    watchData.RecordDateTime = tv.tv_sec;
    watchData.isHasImage = 1;
    uint8_t img[4] = {1, 2, 3, 4};

    char imgBase64[16];
    unsigned int imgBase64Len = 0;

    base64_encode((unsigned char *) img, 4, (unsigned char *) imgBase64, &imgBase64Len);

    watchData.imageData = string(imgBase64).data();

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
//    objTarget1.speed = "很快";

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
//    objTarget2.speed = "很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    string jsonMarshal;

    JsonMarshalWatchData(watchData, jsonMarshal);

    WatchData watchData1;

    JsonUnmarshalWatchData(jsonMarshal, watchData1);


}

#pragma pack(1)
struct S {
    char a;
    int b;
    uint16_t c;
};
#pragma pack()


void initS(struct S *s) {
    for (int i = 0; i < 2; i++) {
        s[i].a = i;
        s[i].b = i + 1;
        s[i].c = i + 2;
    }
}

void printS(struct S *s, int num) {
    for (int i = 0; i < num; i++) {
        printf("a:%d,b:%d,c:%d\n", s[i].a, s[i].b, s[i].c);
    }
}

void testCRC() {
    uint8_t data[] = {0x31, 0x32, 0x33, 0x34, 0x35};
//    uint16_t crc = CRC16_CCITT(data, 4);
    uint16_t crc = Crc16TabCCITT(data, sizeof(data) / sizeof(data[0]));
    cout << "crc:" << to_string(crc) << endl;
}


void exampleJsonTrafficFlow() {
    TrafficFlow trafficFlow;
    trafficFlow.hardCode = "123";
    trafficFlow.oprNum = "123";
    trafficFlow.crossCode = "123";
    trafficFlow.timstamp = 12345678;
    FlowData flowData;
    flowData.laneCode = "1";
    flowData.flowDirection = 0;
    trafficFlow.flowData.push_back(flowData);
    FlowData flowData1;
    flowData1.laneCode = "2";
    flowData1.flowDirection = 1;
    trafficFlow.flowData.push_back(flowData1);
    string body;
    JsonMarshalTrafficFlow(trafficFlow, body);
    cout << "body:" << body << endl;
    TrafficFlow trafficFlow1;
    JsonUnmarshalTrafficFlow(body, trafficFlow1);

}

void exampleJsonTrafficFlows() {
    TrafficFlows trafficFlows;
    trafficFlows.oprNum = "123";
    trafficFlows.crossID = "456";
    trafficFlows.timestamp = 123456789;
    OneRoadTrafficFlow oneRoadTrafficFlow;
    oneRoadTrafficFlow.hardCode = "1";
    oneRoadTrafficFlow.crossCode = "2";
    FlowData flowData;
    flowData.laneCode = "1";
    flowData.flowDirection = 0;
    oneRoadTrafficFlow.flowData.push_back(flowData);
    FlowData flowData1;
    flowData1.laneCode = "2";
    flowData1.flowDirection = 1;
    oneRoadTrafficFlow.flowData.push_back(flowData1);
    trafficFlows.trafficFlow.push_back(flowData1);
    OneRoadTrafficFlow oneRoadTrafficFlow1;
    oneRoadTrafficFlow1.hardCode = "2";
    oneRoadTrafficFlow1.crossCode = "3";
    FlowData flowData2;
    flowData2.laneCode = "3";
    flowData2.flowDirection = 2;
    oneRoadTrafficFlow1.flowData.push_back(flowData2);
    FlowData flowData3;
    flowData3.laneCode = "4";
    flowData3.flowDirection = 3;
    oneRoadTrafficFlow1.flowData.push_back(flowData3);
    trafficFlows.trafficFlow.push_back(flowData3);
    string body;
    JsonMarshalTrafficFlows(trafficFlows, body);
    cout << "body:" << body << endl;
    TrafficFlows trafficFlows1;
    JsonUnmarshalTrafficFlows(body, trafficFlows1);
    Pkg pkg;
    PkgTrafficFlowsWithoutCRC(trafficFlows1, 1, 1, pkg);
    uint8_t data[1024 * 1024];
    uint32_t len;
    common::Pack(pkg, data, &len);

}

void exampleLineupInfo() {
    LineupInfo lineupInfo;
    lineupInfo.OprNum = "123";
    lineupInfo.HardCode = "abc";
    TrafficFlowLineup trafficFlowLineup;
    trafficFlowLineup.HeadWay = 1;
    trafficFlowLineup.AverageSpeed = 2;
    lineupInfo.TrafficFlowList.push_back(trafficFlowLineup);
    TrafficFlowLineup trafficFlowLineup1;
    trafficFlowLineup1.HeadWay = 3;
    trafficFlowLineup1.AverageSpeed = 4;
    lineupInfo.TrafficFlowList.push_back(trafficFlowLineup1);
    Json::FastWriter fastWriter;
    Json::Reader reader;

    Json::Value out;
    lineupInfo.JsonMarshal(out);
    string jsonStr;
    jsonStr = fastWriter.write(out);
    cout << jsonStr << endl;
    LineupInfo lineupInfo1;
    Json::Value in;
    reader.parse(jsonStr, in, false);
    lineupInfo1.JsonUnmarshal(in);
    cout << "ok" << endl;
};

int main(int argc, char **argv) {

//    exampleJsonTrafficFlow();
//    exampleJsonTrafficFlows();
    exampleLineupInfo();
    string a = "nihao";
    string b;
    b = a;
    printf("%s\n", b.c_str());
    a = "hello";
    printf("%s\n", b.c_str());


    testCRC();
    return 0;

//    int a = MEMBER_SIZE(S, b);

//    {
//        S s;
//        s.a = '$';
//        s.b = 11;
//        s.c = 23;
//
//        RingBuffer *rb = RingBuffer_New(1024);
//        RingBuffer_Write(rb, &s, sizeof(s));
//        char a;
//        RingBuffer_Read(rb, &a, MEMBER_SIZE(S, a));
//
//        S s1;
//
//        RingBuffer_Read(rb, &s1.b, (sizeof(s1) - MEMBER_SIZE(S, a)));
//    }

//    Fatal("1234");
//    examplePkg();

//    exampleJsonWatchData();


//    Queue<Pkg> q;
//    Pkg pkg;
//
//    pkg.body.methodName.len = 9;
//    pkg.body.methodName.name = "WatchData";
//
//    q.Push(pkg);
//
//    Pkg pkg1;
//    pkg1 = q.PopFront();

    vector<struct S> vectorS;
    struct S s1 = {
            .a= 1,
            .b = 2,
            .c = 3,
    };
    struct S s2 = {
            .a= 2,
            .b = 3,
            .c = 4,
    };
    vectorS.push_back(s1);
    vectorS.push_back(s2);
    printS(vectorS.data(), vectorS.size());


    return 0;

}
