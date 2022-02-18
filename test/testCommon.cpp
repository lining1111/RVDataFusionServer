//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <queue>
#include <sys/time.h>
#include "common/common.h"
#include "common/CRC.h"
#include "common/Queue.h"
#include "log/Log.h"
#include "ringBuffer/RingBuffer.h"

using namespace common;
using namespace log;

/**
 * 一次帧数据组包解包过程
 */
void examplePkg() {
    Pkg pkg;
    int len = 0;
    //1.头部
    pkg.head.tag = '$';
    pkg.head.version = 1;
    pkg.head.type = PkgType::Request;
    pkg.head.sn = 1;
    pkg.head.len = 0;
    len += sizeof(pkg.head);
    //2.正文
    //2.1方法名
    string methodName = Method(WatchData);
    pkg.body.methodName.len = methodName.length();
    pkg.body.methodName.name = methodName;
    len += sizeof(pkg.body.methodName.len) + pkg.body.methodName.len;

    //2.2方法参数
    string methodParam = "this is a test";
    pkg.body.methodParam.len = methodParam.length();
    pkg.body.methodParam.param = methodParam;
    len += sizeof(pkg.body.methodParam.len) + pkg.body.methodParam.len;
    //3检验值
    pkg.crc.data = Crc16TabCCITT((uint8_t *) &pkg, len);
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
    objTarget1.directionAngle = "西南45度";
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
    objTarget2.directionAngle = "西南45度";
    objTarget2.speed = "很快";

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

int main(int argc, char **argv) {


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
    examplePkg();

//    exampleJsonWatchData();


    Queue<Pkg> q;
    Pkg pkg;

    pkg.body.methodName.len = 9;
    pkg.body.methodName.name = "WatchData";

    q.Push(pkg);

    Pkg pkg1;
    pkg1 = q.PopFront();

    return 0;

}
