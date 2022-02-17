//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <sys/time.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include "common/common.h"

namespace common {

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


    int Pack(Pkg pkg, uint8_t *out, uint32_t *len) {
        int index = 0;

        //1.头部
        memcpy(out + index, &pkg.head, sizeof(pkg.head));
        index += sizeof(pkg.head);
        //2.正文
        //2.1 方法名
        memcpy(out + index, &pkg.body.methodName.len, sizeof(pkg.body.methodName.len));
        index += sizeof(pkg.body.methodName.len);
        memcpy(out + index, pkg.body.methodName.name.data(), pkg.body.methodName.len);
        index += pkg.body.methodName.len;
        //2.1 方法参数
        memcpy(out + index, &pkg.body.methodParam.len, sizeof(pkg.body.methodParam.len));
        index += sizeof(pkg.body.methodParam.len);
        memcpy(out + index, pkg.body.methodParam.param.data(), pkg.body.methodParam.len);
        index += pkg.body.methodParam.len;
        //3.校验值
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
        if (len < sizeof pkg.head) {
            return -1;
        }

        //1.头部
        memcpy(&pkg.head, in + index, sizeof(pkg.head));
        index += sizeof(pkg.head);

        //判断长度，如果 len小于头部长度则退出
        if (len < pkg.head.len) {
            return -1;
        }

        //2.正文
        //2.1方法名
        memcpy(&pkg.body.methodName.len, in + index, sizeof(pkg.body.methodName.len));
        index += sizeof(pkg.body.methodName.len);
        pkg.body.methodName.name.clear();
        pkg.body.methodName.name.append((char *) (in + index), pkg.body.methodName.len);
        index += pkg.body.methodName.len;
        //2.1方法参数
        memcpy(&pkg.body.methodParam.len, in + index, sizeof(pkg.body.methodParam.len));
        index += sizeof(pkg.body.methodParam.len);
        pkg.body.methodParam.param.clear();
        pkg.body.methodParam.param.append((char *) (in + index), pkg.body.methodParam.len);
        index += pkg.body.methodParam.len;
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
        //root AnnuciatorInfo
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

        // root lstObjTarget
        Json::Value arrayObjTarget;
        for (auto iter:watchData.lstObjTarget) {
            Json::Value item;
            //objID
            item["objID"] = iter.objID;
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
            //directionAngle
            item["directionAngle"] = iter.directionAngle;
            //speed
            item["speed"] = iter.speed;

            arrayObjTarget.append(item);
        }
        root["lstObjTarget"] = arrayObjTarget;

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

        //AnnuciatorInfo
        if (!root["AnnuciatorInfo"].isObject()) {
            cout << "json no AnnuciatorInfo" << endl;
            return -1;
        }
        // AnnuciatorInfo list
        for (auto iter: root["AnnuciatorInfo"]) {
            AnnuciatorInfo item;
            item.LightID = iter["LightID"].asInt();
            item.Light = iter["Light"].asString();
            item.RT = iter["RT"].asInt();

            watchData.listAnnuciatorInfo.push_back(item);
        }

        //lstObjTarget
        if (!root["lstObjTarget"].isObject()) {
            cout << "json no lstObjTarget" << endl;
            return -1;
        }
        // lstObjTarget list
        for (auto iter: root["lstObjTarget"]) {
            ObjTarget item;
            item.objID = iter["objID"].asInt();
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
            item.directionAngle = iter["directionAngle"].asString();
            item.speed = iter["speed"].asString();

            watchData.lstObjTarget.push_back(item);
        }

        return 0;
    }


}