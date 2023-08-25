//
// Created by lining on 2/16/22.
//

#include <queue>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include "common/common.h"
#include "common/CRC.h"
#include "Queue.hpp"
#include "ringbuffer/RingBuffer.h"
#include "data/merge/AlgorithmParam.h"


using namespace common;

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
    pkg.head.cmd = CmdType::CmdFusionData;
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
    watchData.timestamp = double(tv.tv_sec);
    watchData.matrixNo = "matrixNo";
    watchData.cameraIp = "192.168.1.100";
    watchData.RecordDateTime = tv.tv_sec;
    watchData.isHasImage = 1;

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
    objTarget1.objID = "1";
    objTarget1.objType = 1;
    objTarget1.plates = "冀A123456";
    objTarget1.plateColor = "BLUE";
    objTarget1.left = 1;
    objTarget1.top = 2;
    objTarget1.right = 3;
    objTarget1.bottom = 4;
    objTarget1.locationX = 5.0;
    objTarget1.locationY = 6.0;
    objTarget1.distance = "很近";
    objTarget1.directionAngle = -45.0;
//    objTarget1.speed = "很快";

    ObjTarget objTarget2;
    objTarget2.objID = "2";
    objTarget2.objType = 2;
    objTarget2.plates = "冀A234567";
    objTarget2.plateColor = "BLUE";
    objTarget2.left = 1;
    objTarget2.top = 2;
    objTarget2.right = 3;
    objTarget2.bottom = 4;
    objTarget2.locationX = 5.0;
    objTarget2.locationY = 6.0;
    objTarget2.distance = "很近";
    objTarget2.directionAngle = -45.0;
//    objTarget2.speed = "很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    string jsonMarshal;
    Json::FastWriter fastWriter;
    Json::Value root;
    watchData.JsonMarshal(root);
    jsonMarshal = fastWriter.write(root);
    std::cout << jsonMarshal << std::endl;

    WatchData watchData1;
    string jsonMarshal1;
    Json::Value root1;
    watchData1.JsonMarshal(root1);
    jsonMarshal1 = fastWriter.write(root);



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

#include <data/merge/mergeStruct.h>
#include <future>
#include <csignal>
#include <fstream>

void exampleAlgorithm() {
    AlgorithmParam algorithmParam;
    //H_south
    double h_sourth[9] = {-0.007128362730, -0.583855347756, 114.889396729743,
                          -0.002529960713, -0.207213146406, 40.775908214483,
                          -0.000062048636, -0.005081861967, 1.000000000000};
    vector<double> h_sourth_values;
    for (auto iter: h_sourth) {
        h_sourth_values.push_back(iter);
    }
    algorithmParam.setH_XX(algorithmParam.H_south, h_sourth_values);
    //H_north
    double h_north[9] = {-0.016188874511, -0.740660413078, 114.891390365757,
                         -0.005745262581, -0.262856666713, 40.772950713216,
                         -0.000140902508, -0.006446667951, 1.000000000000};
    vector<double> h_north_values;
    for (auto iter: h_north) {
        h_north_values.push_back(iter);
    }
    algorithmParam.setH_XX(algorithmParam.H_north, h_north_values);
    //H_east
    double h_east[9] = {1.465148321114, -23.003566173151, 114.847388807580,
                        0.519995524511, -8.163997188248, 40.762179835812,
                        0.012752695973, -0.200223303444, 1.000000000000};
    vector<double> h_east_values;
    for (auto iter: h_east) {
        h_east_values.push_back(iter);
    }
    algorithmParam.setH_XX(algorithmParam.H_east, h_east_values);

    //H_west
    double h_west[9] = {-1.038881614089, 30.397104706610, 114.824762733410,
                        -0.368681781840, 10.787881511903, 40.757922817917,
                        -0.009042351949, 0.264573853716, 1.000000000000};
    vector<double> h_west_values;
    for (auto iter: h_west) {
        h_west_values.push_back(iter);
    }
    algorithmParam.setH_XX(algorithmParam.H_west, h_west_values);

    algorithmParam.crossroad_mid_longitude = 114.8901745;
    algorithmParam.crossroad_mid_latitude = 40.7745085;

    algorithmParam.piexl_type = 1;
    algorithmParam.min_track_distance = 80;// 600 / dst_ratio;// 600;
    algorithmParam.max_track_distance = 120;// 2000 / dst_ratio;// 1400;
    algorithmParam.failCount1 = 50;
    algorithmParam.failCount2 = 2;
    algorithmParam.min_area_threshold = 0.3;
    algorithmParam.piexlbymeter_x = 10.245;
    algorithmParam.piexlbymeter_y = 7.76;
    algorithmParam.road_length = 150;

    vector<double> angles = {180, 270, 0, 103};
    algorithmParam.setAngle(angles);


    algorithmParam.max_stop_speed_threshold = 2.0;//  test
    algorithmParam.shaking_pixel_threshold = 0.3;
    algorithmParam.stopline_length = 5;
    algorithmParam.track_in_length = 120;
    algorithmParam.track_out_length = 20;


    algorithmParam.max_speed_by_piexl = 20; //新增 速度限制
    algorithmParam.car_match_count = 3;
    algorithmParam.max_center_distance_threshold = 0.4;// 0.45f; // 0.4
    algorithmParam.min_center_distance_threshold = 0.2;// 0.25f; //0.2
    algorithmParam.min_area_threshold = 0.1;// 0.45f; //0.5
    algorithmParam.max_area_threshold = 0.3f; // 0.8
    algorithmParam.middle_area_threshold = 0.2;
    //south_north_driving_area
    vector<PointF> values_tmp;
    vector<vector<PointF>> values_driving_area;

    values_driving_area.clear();
    values_tmp.clear();
    values_tmp.push_back(PointF(348, 426));
    values_tmp.push_back(PointF(575, 315));
    values_tmp.push_back(PointF(727, 250));
    values_tmp.push_back(PointF(916, 244));
    values_tmp.push_back(PointF(941, 309));
    values_tmp.push_back(PointF(996, 432));
    values_driving_area.push_back(values_tmp);
    values_tmp.clear();
    values_tmp.push_back(PointF(941, 391));
    values_tmp.push_back(PointF(1245, 293));
    values_tmp.push_back(PointF(1401, 233));
    values_tmp.push_back(PointF(1681, 189));
    values_tmp.push_back(PointF(1631, 285));
    values_tmp.push_back(PointF(1793, 437));
    values_driving_area.push_back(values_tmp);
    algorithmParam.setXX_driving_are(algorithmParam.south_north_driving_area, values_driving_area);
    //north_south_driving_area
    values_driving_area.clear();
    values_tmp.clear();
    values_tmp.push_back(PointF(257, 382));
    values_tmp.push_back(PointF(766, 293));
    values_tmp.push_back(PointF(1102, 222));
    values_tmp.push_back(PointF(1401, 233));
    values_tmp.push_back(PointF(1245, 293));
    values_tmp.push_back(PointF(941, 391));
    values_driving_area.push_back(values_tmp);
    values_tmp.clear();
    values_tmp.push_back(PointF(996, 432));
    values_tmp.push_back(PointF(941, 309));
    values_tmp.push_back(PointF(916, 244));
    values_tmp.push_back(PointF(1113, 247));
    values_tmp.push_back(PointF(1297, 309));
    values_tmp.push_back(PointF(1711, 453));
    values_driving_area.push_back(values_tmp);
    algorithmParam.setXX_driving_are(algorithmParam.north_south_driving_area, values_driving_area);

    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(0, 0.000015, 0.000035));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(1, 0.000015, 0.00004));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(2, 0.000015, 0.000035));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(3, 0.000035, 0.00002));


    Json::FastWriter fastWriter;
    Json::Value out;
    algorithmParam.JsonMarshal(out);
    std::string plainJson;
    plainJson = fastWriter.write(out);
    std::cout << plainJson << std::endl;

    //写入文件
    ofstream file;
    file.open("algorithmParam.json", ios::trunc);
    if (file.is_open()) {
        file << plainJson;
        file.flush();
        file.close();
    }

    Json::Reader reader;
    Json::Value in;
    if (!reader.parse(plainJson, in, false)) {
        std::cout << "json parse fail" << std::endl;
    }

    AlgorithmParam algorithmParam1;
    algorithmParam1.JsonUnmarshal(in);
    std::cout << "angle:" << algorithmParam1.angle.at(0).value << std::endl;
}

int th1(int wait) {
    sleep(wait);
    std::cout << "wait" << wait << std::endl;
    return 0;
}

void aysntest() {

    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < 4; i++) {
        std::shared_future<int> finish = std::async(std::launch::async, th1, 5 - i);
        finishs.push_back(finish);
    }

    for (int i = 0; i < 4; i++) {
        finishs.at(i).wait();
    }
    std::cout << "finish" << std::endl;
}


typedef struct S1 {
    int a = 0;
    string b;
};

int main(int argc, char **argv) {

//    Json::Reader reader;
//    Json::Value in;
//    string content = R"({"loactionX":null})";
//    if (!reader.parse(content,in,false)){
//        return -1;
//    }
//    ObjMix objMix;
//    objMix.JsonUnmarshal(in);
//    Json::FastWriter fastWriter;
//    Json::Value out;
//    objMix.JsonMarshal(out);
//    std::cout << fastWriter.write(out) << std::endl;


//    vector<S1> vs;
//    vs.resize(4);
//    S1 s1;
//    s1.a = 1;
//    s1.b = "b";
//    vs[0]=s1;
//    S1 s2;
//    s2.a = 2;
//    s2.b = "2b";
//    vs[1]=s2;
//    S1 s3;
//    s3.a = 3;
//    s3.b = "3b";
//    vs[2]=s3;
//    for (int i = 0; i < vs.size(); i++) {
//        auto iter = &vs.at(i);
//        printf("a:%d b:%s\n",iter->a,iter->b.c_str());
//    }


    exampleJsonWatchData();
//    aysntest();
    exampleAlgorithm();
//    exampleJsonTrafficFlow();
//    exampleJsonTrafficFlows();
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

//    vector<struct S> vectorS;
//    struct S s1 = {
//            .a= 1,
//            .b = 2,
//            .c = 3,
//    };
//    struct S s2 = {
//            .a= 2,
//            .b = 3,
//            .c = 4,
//    };
//    vectorS.push_back(s1);
//    vectorS.push_back(s2);
//    printS(vectorS.data(), vectorS.size());


    return 0;

}
