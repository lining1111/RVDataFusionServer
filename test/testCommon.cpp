//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <sys/time.h>
#include "common/common.h"
#include "common/CRC.h"

using namespace common;

/**
 * 一次帧数据组包解包过程
 */
void exampleFrame() {
    Frame frame;
    int len = 0;
    //1.头部
    frame.head.tag = '$';
    frame.head.version = 1;
    frame.head.type = FrameType::Request;
    frame.head.sn = 1;
    frame.head.len = 0;
    len += sizeof(frame.head);
    //2.正文
    //2.1方法名
    string methodName = Method(WathData);
    frame.body.methodName.len = methodName.length();
    frame.body.methodName.name = (char *) calloc(1, frame.body.methodName.len);
    methodName.copy(frame.body.methodName.name, frame.body.methodName.len);
    len += sizeof(frame.body.methodName.len) + frame.body.methodName.len;

    //2.2方法参数
    string methodParam = "this is a test";
    frame.body.methodParam.len = methodParam.length();
    frame.body.methodParam.param = (char *) calloc(1, frame.body.methodParam.len);
    methodParam.copy(frame.body.methodParam.param, frame.body.methodParam.len);
    len += sizeof(frame.body.methodParam.len) + frame.body.methodParam.len;
    //3检验值
    frame.crc.data = Crc16TabCCITT((uint8_t *) &frame, len);
    len += sizeof(frame.crc);
    //4 长度信息
    frame.head.len = len;

    //组包
    uint8_t dataEncode[1024 * 1024];
    uint32_t dataEncodeLen = 0;
    bzero(dataEncode, sizeof(dataEncode) / sizeof(dataEncode[0]));
    Pack(frame, dataEncode, &dataEncodeLen);
    ReleaseFrame(frame);
    //解包
    Frame frameDecode;
    Unpack(dataEncode, dataEncodeLen, frameDecode);
    ReleaseFrame(frameDecode);
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
    annuciatorInfo1.LightID =1;
    annuciatorInfo1.Light = "R";
    annuciatorInfo1.RT = 123;

    AnnuciatorInfo annuciatorInfo2;
    annuciatorInfo2.LightID =2;
    annuciatorInfo2.Light = "G";
    annuciatorInfo2.RT = 456;

    AnnuciatorInfo annuciatorInfo3;
    annuciatorInfo3.LightID =3;
    annuciatorInfo3.Light = "Y";
    annuciatorInfo3.RT = 789;
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo1);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo2);
    watchData.listAnnuciatorInfo.push_back(annuciatorInfo3);

    //lstObjTarget

    ObjTarget objTarget1;
    objTarget1.objID = 1;
    objTarget1.objType = 1;
    objTarget1.plates="冀A123456";
    objTarget1.plateColor = "BLUE";
    objTarget1.left = 1;
    objTarget1.top =2;
    objTarget1.right=3;
    objTarget1.bottom =4;
    objTarget1.locationX = 5;
    objTarget1.locationY =6;
    objTarget1.distance="很近";
    objTarget1.directionAngle="西南45度";
    objTarget1.speed="很快";

    ObjTarget objTarget2;
    objTarget2.objID = 2;
    objTarget2.objType = 2;
    objTarget2.plates="冀A234567";
    objTarget2.plateColor = "BLUE";
    objTarget2.left = 1;
    objTarget2.top =2;
    objTarget2.right=3;
    objTarget2.bottom =4;
    objTarget2.locationX = 5;
    objTarget2.locationY =6;
    objTarget2.distance="很近";
    objTarget2.directionAngle="西南45度";
    objTarget2.speed="很快";

    watchData.lstObjTarget.push_back(objTarget1);
    watchData.lstObjTarget.push_back(objTarget2);

    string jsonMarshal;

    JsonMarshalWatchData(watchData,jsonMarshal);

    WatchData  watchData1;

    JsonUnmarshalWatchData(jsonMarshal,watchData1);


}

int main(int argc, char **argv) {

//    exampleFrame();

    exampleJsonWatchData();

    return 0;

}
