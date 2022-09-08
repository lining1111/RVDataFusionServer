//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <sys/time.h>

#ifdef x86

#include <jsoncpp/json/json.h>

#else

#include <json/json.h>

#endif

#include <iostream>
#include "common/common.h"
#include "common/CRC.h"

namespace common {

    void PrintHex(uint8_t *data, uint32_t len) {
        int count = 0;
        for (int i = 0; i < len; i++) {
            printf("%02x ", data[i]);
            count++;
            if (count == 16) {
                printf("\n");
                count = 0;
            }
        }
        printf("\n");
    }

    void
    base64_encode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length) {

        if (input == nullptr || input_length == 0 || output == nullptr || output_length == nullptr) {
            return;
        }

        const char encodeCharacterTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        char buff1[3];
        char buff2[4];
        unsigned char i = 0, j;
        unsigned int input_cnt = 0;
        unsigned int output_cnt = 0;

        while (input_cnt < input_length) {
            buff1[i++] = input[input_cnt++];
            if (i == 3) {
                output[output_cnt++] = encodeCharacterTable[(buff1[0] & 0xfc) >> 2];
                output[output_cnt++] = encodeCharacterTable[((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4)];
                output[output_cnt++] = encodeCharacterTable[((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6)];
                output[output_cnt++] = encodeCharacterTable[buff1[2] & 0x3f];
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 3; j++) {
                buff1[j] = '\0';
            }
            buff2[0] = (buff1[0] & 0xfc) >> 2;
            buff2[1] = ((buff1[0] & 0x03) << 4) + ((buff1[1] & 0xf0) >> 4);
            buff2[2] = ((buff1[1] & 0x0f) << 2) + ((buff1[2] & 0xc0) >> 6);
            buff2[3] = buff1[2] & 0x3f;
            for (j = 0; j < (i + 1); j++) {
                output[output_cnt++] = encodeCharacterTable[buff2[j]];
            }
            while (i++ < 3) {
                output[output_cnt++] = '=';
            }
        }
        output[output_cnt] = '\0';

        *output_length = output_cnt;
    }


    void
    base64_decode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length) {

        if (input == nullptr || input_length == 0 || output == nullptr || output_length == nullptr) {
            return;
        }

        const signed char decodeCharacterTable[256] = {
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
                -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
                -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };

        char buff1[4];
        char buff2[4];
        unsigned char i = 0, j;
        unsigned int input_cnt = 0;
        unsigned int output_cnt = 0;

        while (input_cnt < input_length) {
            buff2[i] = input[input_cnt++];
            if (buff2[i] == '=') {
                break;
            }
            if (++i == 4) {
                for (i = 0; i != 4; i++) {
                    buff2[i] = decodeCharacterTable[buff2[i]];
                }
                output[output_cnt++] = (char) ((buff2[0] << 2) + ((buff2[1] & 0x30) >> 4));
                output[output_cnt++] = (char) (((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2));
                output[output_cnt++] = (char) (((buff2[2] & 0x3) << 6) + buff2[3]);
                i = 0;
            }
        }
        if (i) {
            for (j = i; j < 4; j++) {
                buff2[j] = '\0';
            }
            for (j = 0; j < 4; j++) {
                buff2[j] = decodeCharacterTable[buff2[j]];
            }
            buff1[0] = (buff2[0] << 2) + ((buff2[1] & 0x30) >> 4);
            buff1[1] = ((buff2[1] & 0xf) << 4) + ((buff2[2] & 0x3c) >> 2);
            buff1[2] = ((buff2[2] & 0x3) << 6) + buff2[3];
            for (j = 0; j < (i - 1); j++) {
                output[output_cnt++] = (char) buff1[j];
            }
        }
        *output_length = output_cnt;

    }


    /*
 * 函数功能：产生uuid
 * 参数：无
 * 返回值：uuid的string
 * */
    string random_uuid(string seed) {
        char buf[37] = {0};
        struct timeval tmp;
        const char *c = seed.data();//"89ab"
        char *p = buf;
        unsigned int n, b;
        gettimeofday(&tmp, NULL);
        srand(tmp.tv_usec);

        for (n = 0; n < 16; ++n) {
            b = rand() % 65536;
            switch (n) {
                case 6:
                    sprintf(p, "4%x", b % 15);
                    break;
                case 8:
                    sprintf(p, "%c%x", c[rand() % strlen(c)], b % 15);
                    break;
                default:
                    sprintf(p, "%02x", b);
                    break;
            }
            p += 2;
            switch (n) {
                case 3:
                case 5:
                case 7:
                case 9:
                    *p++ = '-';
                    break;
            }
        }
        *p = 0;
        return string(buf);
    }

    int Pack(Pkg &pkg, uint8_t *out, uint32_t *len) {
        int index = 0;

        //1.头部
        memcpy(out + index, &pkg.head, sizeof(pkg.head));
        index += sizeof(pkg.head);
        //2.正文 string
        memcpy(out + index, pkg.body.data(), pkg.body.length());
        index += pkg.body.length();
        //3.校验值先计算再更新
        pkg.crc.data = Crc16TabCCITT(out, index);
        memcpy(out + index, &pkg.crc, sizeof(pkg.crc));
        index += sizeof(pkg.crc);
        //4.设置长度
        *len = pkg.head.len;

        //如果最后拷贝的长度和头部信息的长度相同则说明组包成功，否则失败
        if (index != pkg.head.len) {
            return -1;
        } else {
            return 0;
        }
    }


    int Unpack(uint8_t *in, uint32_t len, Pkg &pkg) {
        int index = 0;

        //长度小于头部长度 退出
        if (len < sizeof(pkg.head)) {
            cout << "长度小于头部长度" << endl;
            return -1;
        }

        //1.头部
        memcpy(&pkg.head, in + index, sizeof(pkg.head));
        index += sizeof(pkg.head);

        //判断长度，如果 len小于头部长度则退出
        if (len < pkg.head.len) {
            cout << "长度小于数据长度" << endl;
            return -1;
        }

        //2.正文string
        pkg.body.clear();
        pkg.body.assign((char *) (in + index), (pkg.head.len - sizeof(pkg.head) - sizeof(pkg.crc)));
        index += (pkg.head.len - sizeof(pkg.head) - sizeof(pkg.crc));
        //3.校验值
        memcpy(&pkg.crc, in + index, sizeof(pkg.crc));
        index += sizeof(pkg.crc);

        //如果最后拷贝的长度和参数长度相同则说明组包成功，否则失败
        if (index != len) {
            return -1;
        } else {
            return 0;
        }

    }

    int JsonMarshalWatchData(WatchData watchData, string &out) {
        Json::FastWriter fastWriter;
        Json::Value root;

        //root oprNum
        root["oprNum"] = watchData.oprNum;
        //root hardCode
        root["hardCode"] = watchData.hardCode;
        //root timstamp
        root["timstamp"] = watchData.timstamp;
        //root matrixNo
        root["matrixNo"] = watchData.matrixNo;
        //root cameraIp
        root["cameraIp"] = watchData.cameraIp;
        //root RecordDateTime
        root["RecordDateTime"] = watchData.RecordDateTime;
        //root isHasImage
        root["isHasImage"] = watchData.isHasImage;
        //root imageData
        root["imageData"] = watchData.imageData;
        //root direction
        root["direction"] = watchData.direction;

        //root AnnuciatorInfo
        if (!watchData.listAnnuciatorInfo.empty()) {
            Json::Value arrayAnnuciatorInfo;
            for (auto iter: watchData.listAnnuciatorInfo) {
                Json::Value item;
                // LightID
                item["LightID"] = iter.LightID;
                //Light
                item["Light"] = iter.Light;
                //RT
                item["RT"] = iter.RT;
                arrayAnnuciatorInfo.append(item);
            }
            root["AnnuciatorInfo"] = arrayAnnuciatorInfo;
        }

        // root lstObjTarget
        if (!watchData.lstObjTarget.empty()) {
            Json::Value arrayObjTarget;
            for (auto iter:watchData.lstObjTarget) {
                Json::Value item;
                //objID
                item["objID"] = iter.objID;
                //objCameraID
                item["objCameraID"] = iter.objCameraID;
                //objType
                item["objType"] = iter.objType;
                //plates
                item["plates"] = iter.plates;
                //plateColor
                item["plateColor"] = iter.plateColor;
                //left
                item["left"] = iter.left;
                //top
                item["top"] = iter.top;
                //right
                item["right"] = iter.right;
                //bottom
                item["bottom"] = iter.bottom;
                //locationX
                item["locationX"] = iter.locationX;
                //locationY
                item["locationY"] = iter.locationY;
                //distance
                item["distance"] = iter.distance;
                //angle
                item["directionAngle"] = iter.directionAngle;
//                //speed
//                item["speed"] = iter.speed;
                //speedX
                item["speedX"] = iter.speedX;
                //speedY
                item["speedY"] = iter.speedY;
                //longitude
                item["longitude"] = iter.longitude;
                //latitude
                item["latitude"] = iter.latitude;

                arrayObjTarget.append(item);
            }
            root["lstObjTarget"] = arrayObjTarget;
        }

        out = fastWriter.write(root);

        return 0;
    }

    int JsonUnmarshalWatchData(string in, WatchData &watchData) {
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(in, root, false)) {
            cout << "not json drop" << endl;
            return -1;
        }

        //oprNum
        watchData.oprNum = root["oprNum"].asString();
        //hardCode
        watchData.hardCode = root["hardCode"].asString();
        //timstamp
        watchData.timstamp = root["timstamp"].asDouble();
        //matrixNo
        watchData.matrixNo = root["matrixNo"].asString();
        //cameraIp
        watchData.cameraIp = root["cameraIp"].asString();
        //RecordDateTime
        watchData.RecordDateTime = root["RecordDateTime"].asDouble();
        //isHasImage
        watchData.isHasImage = root["isHasImage"].asInt();
        //imageData
        watchData.imageData = root["imageData"].asString();
        //direction
        watchData.direction = root["direction"].asInt();

        //AnnuciatorInfo
        if (!root["AnnuciatorInfo"].isArray()) {
//            cout << "json no AnnuciatorInfo" << endl;
        } else {
            // AnnuciatorInfo list
            for (auto iter: root["AnnuciatorInfo"]) {
                AnnuciatorInfo item;
                item.LightID = iter["LightID"].asInt();
                item.Light = iter["Light"].asString();
                item.RT = iter["RT"].asInt();

                watchData.listAnnuciatorInfo.push_back(item);
            }
        }

        //lstObjTarget
        if (!root["lstObjTarget"].isArray()) {
//            cout << "json no lstObjTarget" << endl;
        } else {
            // lstObjTarget list
            for (auto iter: root["lstObjTarget"]) {
                ObjTarget item;
                item.objID = iter["objID"].asInt();
                item.objCameraID = iter["objCameraID"].asInt();
                item.objType = iter["objType"].asInt();
                item.plates = iter["plates"].asString();
                item.plateColor = iter["plateColor"].asString();
                item.left = iter["left"].asInt();
                item.top = iter["top"].asInt();
                item.right = iter["right"].asInt();
                item.bottom = iter["bottom"].asInt();
                item.locationX = iter["locationX"].asDouble();
                item.locationY = iter["locationY"].asDouble();
                item.distance = iter["distance"].asString();
                item.directionAngle = iter["directionAngle"].asDouble();
//                item.speed = iter["speed"].asString();
                item.speedX = iter["speedX"].asDouble();
                item.speedY = iter["speedY"].asDouble();
                item.longitude = iter["longitude"].asDouble();
                item.latitude = iter["latitude"].asDouble();

                watchData.lstObjTarget.push_back(item);
            }
        }

        return 0;
    }

    int PkgWatchDataWithoutCRC(WatchData watchData, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceData;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        JsonMarshalWatchData(watchData, jsonStr);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }

    int JsonMarshalFusionData(FusionData fusionData, string &out) {
        Json::FastWriter fastWriter;
        Json::Value root;

        //root oprNum
        root["oprNum"] = fusionData.oprNum;
        //root timstamp
        root["timstamp"] = fusionData.timstamp;
        //root crossID
        root["crossID"] = fusionData.crossID;
        //root isHasImage
        root["isHasImage"] = fusionData.isHasImage;
//        //root imageData
//        root["imageData"] = fusionData.imageData;

        //root lstVideos
        if (!fusionData.lstVideos.empty()) {
            Json::Value arrayVideos;
            for (auto iter1:fusionData.lstVideos) {
                Json::Value itemVideos;
                //rvHardCode
                itemVideos["rvHardCode"] = iter1.rvHardCode;
                //imageData
                itemVideos["imageData"] = iter1.imageData;

                //lstVideoTargets
                if (!iter1.lstVideoTargets.empty()) {
                    Json::Value arrayVideoTargets;
                    for (auto iter2:iter1.lstVideoTargets) {
                        Json::Value itemVideoTargets;
                        //cameraObjID
                        itemVideoTargets["cameraObjID"] = iter2.cameraObjID;
                        //left
                        itemVideoTargets["left"] = iter2.left;
                        //top
                        itemVideoTargets["top"] = iter2.top;
                        //right
                        itemVideoTargets["right"] = iter2.right;
                        //bottom
                        itemVideoTargets["bottom"] = iter2.bottom;

                        arrayVideoTargets.append(itemVideoTargets);
                    }
                    itemVideos["lstVideoTargets"] = arrayVideoTargets;
                } else {
                    itemVideos["lstVideoTargets"].resize(0);
                }
                arrayVideos.append(itemVideos);
            }
            root["lstVideos"] = arrayVideos;
        } else {
            root["lstVideos"].resize(0);
        }

        // root lstObjTarget
        if (!fusionData.lstObjTarget.empty()) {
            Json::Value arrayObjTarget;
            for (auto iter:fusionData.lstObjTarget) {
                Json::Value item;
                //objID
                item["objID"] = iter.objID;
                //cameraObjID
//                item["cameraObjID"] = iter.cameraObjID;
                if (!iter.listRvWayObject.empty()) {
                    Json::Value arrayObjTarget1;
                    for (auto iter1:iter.listRvWayObject) {
                        Json::Value item1;
                        //wayNo
                        item1["wayNo"] = iter1.wayNo;
                        //roID
                        item1["roID"] = iter1.roID;
                        //voID
                        item1["voID"] = iter1.voID;
                        arrayObjTarget1.append(item1);
                    }
                    item["rvWayObject"] = arrayObjTarget1;
                } else {
                    item["rvWayObject"].resize(0);
                }

                //objType
                item["objType"] = iter.objType;
                //objColor
                item["objColor"] = iter.objColor;
                //plates
                item["plates"] = iter.plates;
                //plateColor
                item["plateColor"] = iter.plateColor;
//                //left
//                item["left"] = iter.left;
//                //top
//                item["top"] = iter.top;
//                //right
//                item["right"] = iter.right;
//                //bottom
//                item["bottom"] = iter.bottom;
                //distance
                item["distance"] = iter.distance;
                //angle
                item["angle"] = iter.angle;
                //speed
                item["speed"] = iter.speed;
                //locationX
                item["locationX"] = iter.locationX;
                //locationY
                item["locationY"] = iter.locationY;
                //longitude
                item["longitude"] = iter.longitude;
                //latitude
                item["latitude"] = iter.latitude;
                item["flagNew"] = iter.flagNew;

                arrayObjTarget.append(item);
            }
            root["lstObjTarget"] = arrayObjTarget;
        } else {
            root["lstObjTarget"].resize(0);
        }

        out = fastWriter.write(root);

        return 0;
    }

    int JsonUnmarshalFusionData(string in, FusionData &fusionData) {
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(in, root, false)) {
            cout << "not json drop" << endl;
            return -1;
        }

        //oprNum
        fusionData.oprNum = root["oprNum"].asString();
        //timstamp
        fusionData.timstamp = root["timstamp"].asDouble();
        //crossID
        fusionData.crossID = root["crossID"].asString();
        //isHasImage
        fusionData.isHasImage = root["isHasImage"].asInt();
//        //imageData
//        fusionData.imageData = root["imageData"].asString();

        //lstVideos
        if (!root["lstVideos"].isArray()) {
            cout << "json no lstVideos" << endl;
        } else {
            for (auto iter:root["lstVideos"]) {
                VideoData itemVideoData;
                itemVideoData.rvHardCode = iter["rvHardCode"].asString();
                itemVideoData.imageData = iter["imageData"].asString();
                if (iter["lstVideoTargets"].isArray()) {
                    for (auto iter2:iter["lstVideoTargets"]) {
                        VideoTargets itemVideoTargets;
                        itemVideoTargets.cameraObjID = iter2["cameraObjID"].asInt();
                        itemVideoTargets.left = iter2["left"].asInt();
                        itemVideoTargets.top = iter2["top"].asInt();
                        itemVideoTargets.right = iter2["right"].asInt();
                        itemVideoTargets.bottom = iter2["bottom"].asInt();

                        itemVideoData.lstVideoTargets.push_back(itemVideoTargets);
                    }
                }
                fusionData.lstVideos.push_back(itemVideoData);
            }
        }


        //lstObjTarget
        if (!root["lstObjTarget"].isArray()) {
            cout << "json no lstObjTarget" << endl;
        } else {
            // lstObjTarget list
            for (auto iter: root["lstObjTarget"]) {
                ObjMix item;
                item.objID = iter["objID"].asInt();

                if (iter["rvWayObject"].isArray()) {
                    for (auto iter1:iter["rvWayObject"]) {
                        RvWayObject itemRvWayObject;
                        itemRvWayObject.wayNo = iter1["wayNo"].asInt();
                        itemRvWayObject.roID = iter1["roID"].asInt();
                        itemRvWayObject.voID = iter1["voID"].asInt();
                        item.listRvWayObject.push_back(itemRvWayObject);
                    }
                }
//                item.cameraObjID = iter["cameraObjID"].asInt();
                item.objType = iter["objType"].asInt();
                item.objColor = iter["objColor"].asInt();
                item.plates = iter["plates"].asString();
                item.plateColor = iter["plateColor"].asString();
//                item.left = iter["left"].asInt();
//                item.top = iter["top"].asInt();
//                item.right = iter["right"].asInt();
//                item.bottom = iter["bottom"].asInt();
                item.distance = iter["distance"].asFloat();
                item.angle = iter["angle"].asFloat();
                item.speed = iter["speed"].asFloat();
                item.locationX = iter["locationX"].asDouble();
                item.locationY = iter["locationY"].asDouble();
                item.longitude = iter["longitude"].asDouble();
                item.latitude = iter["latitude"].asDouble();
                item.flagNew = iter["flagNew"].asInt();


                fusionData.lstObjTarget.push_back(item);
            }
        }

        return 0;
    }

    int PkgFusionDataWithoutCRC(FusionData fusionData, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceData;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        JsonMarshalFusionData(fusionData, jsonStr);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }

    int JsonMarshalTrafficFlow(TrafficFlow trafficFlow, string &out) {
        Json::FastWriter fastWriter;
        Json::Value root;

        //root oprNum
        root["oprNum"] = trafficFlow.oprNum;
        //root hardCode
        root["hardCode"] = trafficFlow.hardCode;
        //root timstamp
        root["timstamp"] = trafficFlow.timstamp;
        //root crossCode
        root["crossCode"] = trafficFlow.crossCode;

        //root flowData
        if (!trafficFlow.flowData.empty()) {
            Json::Value arrayFlowData;
            for (auto iter:trafficFlow.flowData) {
                Json::Value item;
                item["laneCode"] = iter.laneCode;
                item["flowDirection"] = iter.flowDirection;
                item["inCars"] = iter.inCars;
                item["outCars"] = iter.outCars;
                item["queueLen"] = iter.queueLen;
                item["queueCars"] = iter.queueCars;

                arrayFlowData.append(item);
            }
            root["flowData"] = arrayFlowData;
        } else {
            root["flowData"].resize(0);
        }

        out = fastWriter.write(root);
        return 0;
    }

    int JsonUnmarshalTrafficFlow(string in, TrafficFlow &trafficFlow) {
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(in, root, false)) {
            cout << "not json drop" << endl;
            return -1;
        }

        //oprNum
        trafficFlow.oprNum = root["oprNum"].asString();
        //hardCode
        trafficFlow.hardCode = root["hardCode"].asString();
        //timstamp
        trafficFlow.timstamp = root["timstamp"].asDouble();
        //crossCode
        trafficFlow.crossCode = root["crossCode"].asString();

        if (!root["flowData"].isArray()) {

        } else {
            for (auto iter:root["flowData"]) {
                FlowData flowData;
                flowData.laneCode = iter["laneCode"].asString();
                flowData.flowDirection = iter["flowDirection"].asInt();
                flowData.inCars = iter["inCars"].asInt();
                flowData.outCars = iter["outCars"].asInt();
                flowData.queueLen = iter["queueLen"].asInt();
                flowData.queueCars = iter["queueCars"].asInt();

                trafficFlow.flowData.push_back(flowData);
            }
        }

        return 0;
    }

    int JsonMarshalTrafficFlows(TrafficFlows trafficFlows, string &out) {
        Json::FastWriter fastWriter;
        Json::Value root;

        //root oprNum
        root["oprNum"] = trafficFlows.oprNum;
        //root crossID
        root["crossID"] = trafficFlows.crossID;
        //root timestamp
        root["timestamp"] = trafficFlows.timestamp;

        if (!trafficFlows.trafficFlow.empty()) {
            Json::Value arrayTrafficFlow;
            for (auto iter:trafficFlows.trafficFlow) {
                Json::Value item;
                item["hardCode"] = iter.hardCode;
                item["crossCode"] = iter.crossCode;
                if (!iter.flowData.empty()) {
                    Json::Value arrayFlowData;
                    for (auto iter1:iter.flowData) {
                        Json::Value item1;
                        item1["laneCode"] = iter1.laneCode;
                        item1["flowDirection"] = iter1.flowDirection;
                        item1["inCars"] = iter1.inCars;
                        item1["outCars"] = iter1.outCars;
                        item1["queueLen"] = iter1.queueLen;
                        item1["queueCars"] = iter1.queueCars;

                        arrayFlowData.append(item1);
                    }
                    item["flowData"] = arrayFlowData;
                } else {
                    item["flowData"].resize(0);
                }

                arrayTrafficFlow.append(item);;
            }

            root["trafficFlow"] = arrayTrafficFlow;
        } else {
            root["trafficFlow"].resize(0);
        }

        out = fastWriter.write(root);

        return 0;
    }

    int JsonUnmarshalTrafficFlows(string in, TrafficFlows &trafficFlows) {
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(in, root, false)) {
            cout << "not json drop" << endl;
            return -1;
        }

        //oprNum
        trafficFlows.oprNum = root["oprNum"].asString();
        //crossID
        trafficFlows.crossID = root["crossID"].asString();
        //timstamp
        trafficFlows.timestamp = root["timestamp"].asDouble();

        if (!root["trafficFlow"].isArray()) {

        } else {
            for (auto iter:root["trafficFlow"]) {

                OneRoadTrafficFlow oneRoadTrafficFlow;
                oneRoadTrafficFlow.hardCode = iter["hardCode"].asString();
                oneRoadTrafficFlow.crossCode = iter["corssCode"].asString();
                if (iter["flowData"].isArray()) {
                    for (auto iter1:iter["flowData"]) {
                        FlowData flowData;
                        flowData.laneCode = iter1["laneCode"].asString();
                        flowData.flowDirection = iter1["flowDirection"].asInt();
                        flowData.inCars = iter1["inCars"].asInt();
                        flowData.outCars = iter1["outCars"].asInt();
                        flowData.queueLen = iter1["queueLen"].asInt();
                        flowData.queueCars = iter1["queueCars"].asInt();

                        oneRoadTrafficFlow.flowData.push_back(flowData);
                    }
                }

                trafficFlows.trafficFlow.push_back(oneRoadTrafficFlow);
            }
        }

        return 0;
    }

    int PkgTrafficFlowsWithoutCRC(TrafficFlows trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceMultiView;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        JsonMarshalTrafficFlows(trafficFlows, jsonStr);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }


}