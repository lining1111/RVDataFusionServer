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
#include "data/DataUnit.h"


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
    objTarget2.locationX = 5.12345678745;
    objTarget2.locationY = 6.98974545451;
    objTarget2.distance = "很近";
    objTarget2.directionAngle = -45.0;
//    objTarget2.speed = "很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    Pkg pkg;
    watchData.PkgWithoutCRC(1,12,pkg);

    string jsonMarshal = json::encode(watchData);

    std::cout << jsonMarshal << std::endl;

    WatchData watchData1;
    string jsonMarshal1;
    try {
        json::decode(jsonMarshal, watchData1);
    } catch (std::exception &e) {
        cout << e.what() << endl;
    }
    jsonMarshal1 = json::encode(watchData1);

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
    algorithmParam.intersectionName = "测试路口";
    DoubleValueArray transformMatrixItem;
    //H_south
    double h_sourth[9] = {-0.007128362730, -0.583855347756, 114.889396729743,
                          -0.002529960713, -0.207213146406, 40.775908214483,
                          -0.000062048636, -0.005081861967, 1.000000000000};
    vector<double> h_sourth_values;
    for (auto iter: h_sourth) {
        h_sourth_values.push_back(iter);
    }
    transformMatrixItem.set(0, h_sourth_values);
    algorithmParam.transformMatrix.push_back(transformMatrixItem);
    //H_north
    double h_north[9] = {-0.016188874511, -0.740660413078, 114.891390365757,
                         -0.005745262581, -0.262856666713, 40.772950713216,
                         -0.000140902508, -0.006446667951, 1.000000000000};
    vector<double> h_north_values;
    for (auto iter: h_north) {
        h_north_values.push_back(iter);
    }
    transformMatrixItem.set(1, h_north_values);
    algorithmParam.transformMatrix.push_back(transformMatrixItem);
    //H_east
    double h_east[9] = {1.465148321114, -23.003566173151, 114.847388807580,
                        0.519995524511, -8.163997188248, 40.762179835812,
                        0.012752695973, -0.200223303444, 1.000000000000};
    vector<double> h_east_values;
    for (auto iter: h_east) {
        h_east_values.push_back(iter);
    }
    transformMatrixItem.set(2, h_east_values);
    algorithmParam.transformMatrix.push_back(transformMatrixItem);

    //H_west
    double h_west[9] = {-1.038881614089, 30.397104706610, 114.824762733410,
                        -0.368681781840, 10.787881511903, 40.757922817917,
                        -0.009042351949, 0.264573853716, 1.000000000000};
    vector<double> h_west_values;
    for (auto iter: h_west) {
        h_west_values.push_back(iter);
    }
    transformMatrixItem.set(3, h_west_values);
    algorithmParam.transformMatrix.push_back(transformMatrixItem);

    algorithmParam.midLongitude = 114.8901745;
    algorithmParam.midLatitude = 40.7745085;

    algorithmParam.piexlType = 1;
    algorithmParam.minTrackDistance = 80;// 600 / dst_ratio;// 600;
    algorithmParam.maxTrackDistance = 120;// 2000 / dst_ratio;// 1400;
    algorithmParam.failCount1 = 50;
    algorithmParam.failCount2 = 2;
    algorithmParam.minAreaThreshold = 0.3;
    algorithmParam.piexlByMeterX = 10.245;
    algorithmParam.piexlByMeterY = 7.76;
    algorithmParam.roadLength = 150;

    vector<double> angles = {180, 270, 0, 103};
    for (int i = 0; i < angles.size(); i++) {
        DoubleValue value;
        value.index = i;
        value.value = angles[i];
        algorithmParam.angle.push_back(value);
    }


    algorithmParam.maxStopSpeedThreshold = 2.0;//  test
    algorithmParam.shakingPixelThreshold = 0.3;
    algorithmParam.stoplineLength = 5;
    algorithmParam.trackInLength = 120;
    algorithmParam.trackOutLength = 20;


    algorithmParam.maxSpeedByPiexl = 20; //新增 速度限制
    algorithmParam.carMatchCount = 3;
    algorithmParam.maxCenterDistanceThreshold = 0.4;// 0.45f; // 0.4
    algorithmParam.minCenterDistanceThreshold = 0.2;// 0.25f; //0.2
    algorithmParam.minAreaThreshold = 0.1;// 0.45f; //0.5
    algorithmParam.maxAreaThreshold = 0.3f; // 0.8
    algorithmParam.middleAreaThreshold = 0.2;
    //south_north_driving_area

    PointFArrayArray pointFArrayArrayItem;
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
    pointFArrayArrayItem.set(0, values_driving_area);
    algorithmParam.drivingArea.push_back(pointFArrayArrayItem);
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
    pointFArrayArrayItem.set(1, values_driving_area);
    algorithmParam.drivingArea.push_back(pointFArrayArrayItem);

    RectFValueArray rectFValueArrayItem;
    vector<RectF> values_tmp_rect;
    values_tmp_rect.push_back(RectF( 0.000015, 0.000035,0.0002,0.2555));
    values_tmp_rect.push_back(RectF( 0.000015, 0.00004,0.54545,0.2555));
    values_tmp_rect.push_back(RectF( 0.000015, 0.000035,0.0002,0.2555));
    rectFValueArrayItem.set(0, values_tmp_rect);
    algorithmParam.drivingInArea.push_back(rectFValueArrayItem);
    algorithmParam.drivingMissingArea.push_back(rectFValueArrayItem);
    rectFValueArrayItem.set(1, values_tmp_rect);
    algorithmParam.drivingInArea.push_back(rectFValueArrayItem);
    algorithmParam.drivingMissingArea.push_back(rectFValueArrayItem);

    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(0, 0.000015, 0.000035));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(1, 0.000015, 0.00004));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(2, 0.000015, 0.000035));
    algorithmParam.correctedValueGPSs.push_back(CorrectedValueGPS(3, 0.000035, 0.00002));


    std::string plainJson = json::encode(algorithmParam);
    std::cout << plainJson << std::endl;

    //写入文件
    ofstream file;
    file.open("algorithmParam.json", ios::trunc);
    if (file.is_open()) {
        file << plainJson;
        file.flush();
        file.close();
    }

    AlgorithmParam algorithmParam1;
    try {
        json::decode(plainJson, algorithmParam1);
    } catch (std::exception &e) {
        cout << e.what() << endl;
    }
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

void example1(){
    vector<OneFlowData> v_src;
    OneFlowData item;
    item.laneCode = "l1";
    item.queueLen = 2;
    v_src.push_back(item);
    OneFlowData item1;
    item1.laneCode = "l2";
    item1.queueLen = 5;
    v_src.push_back(item1);
    OneFlowData item2;
    item2.laneCode = "l1";
    item2.queueLen = 6;
    v_src.push_back(item2);
    OneFlowData item3;
    item3.laneCode = "l2";
    item3.queueLen = 2;
    v_src.push_back(item3);

    DataUnitTrafficFlowGather::getMaxQueueLenByLaneCode(v_src);
}
class A{
public:
    vector<vector<vector<double>>> v;
XPACK(O(v));
};
int main(int argc, char **argv) {

    string dataStr =
            "{\"v\":[[[1.0,2.0],[4.0,5.0]],[[7.0,8.0],[10.0,11.0]]]}";

    A a1;
    json::decode(dataStr, a1);

    example1();
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
