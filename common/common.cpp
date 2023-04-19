//
// Created by lining on 2/16/22.
//

#include <cstring>
#include <sys/time.h>
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
        gettimeofday(&tmp, nullptr);
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

    int PkgClass::PkgWithoutCRC(uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = this->cmdType;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        Json::FastWriter fastWriter;
        Json::Value root;
        this->JsonMarshal(root);
        jsonStr = fastWriter.write(root);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);
        pkg.head.len = len;
        return 0;
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


    bool AnnuciatorInfo::JsonMarshal(Json::Value &out) {

        out["LightID"] = this->LightID;
        out["Light"] = this->Light;
        out["RT"] = this->RT;

        return true;
    }

    bool AnnuciatorInfo::JsonUnmarshal(Json::Value in) {
        this->LightID = in["LightID"].asInt();
        this->Light = in["Light"].asString();
        this->RT = in["RT"].asInt();

        return true;
    }

    bool ObjTarget::JsonMarshal(Json::Value &out) {
        out["objID"] = this->objID;
        out["objCameraID"] = this->objCameraID;
        out["objRadarID"] = this->objRadarID;
        out["objSourceType"] = this->objSourceType;
        out["objType"] = this->objType;
        out["plates"] = this->plates;
        out["plateColor"] = this->plateColor;
        out["left"] = this->left;
        out["top"] = this->top;
        out["right"] = this->right;
        out["bottom"] = this->bottom;
        out["locationX"] = this->locationX;
        out["locationY"] = this->locationY;
        out["distance"] = this->distance;
        out["directionAngle"] = this->directionAngle;
        out["speedX"] = this->speedX;
        out["speedY"] = this->speedY;
        out["longitude"] = this->longitude;
        out["latitude"] = this->latitude;
        out["laneCode"] = this->laneCode;
        out["carLength"] = this->carLength;
        out["carFeaturePic"] = this->carFeaturePic;
        return true;
    }

    bool ObjTarget::JsonUnmarshal(Json::Value in) {
        this->objID = in["objID"].asInt();
        this->objCameraID = in["objCameraID"].asInt();
        this->objRadarID = in["objRadarID"].asInt();
        this->objSourceType = in["objSourceType"].asInt();
        this->objType = in["objType"].asInt();
        this->plates = in["plates"].asString();
        this->plateColor = in["plateColor"].asString();
        this->left = in["left"].asInt();
        this->top = in["top"].asInt();
        this->right = in["right"].asInt();
        this->bottom = in["bottom"].asInt();
        this->locationX = in["locationX"].asDouble();
        this->locationY = in["locationY"].asDouble();
        this->distance = in["distance"].asString();
        this->directionAngle = in["directionAngle"].asDouble();
        this->speedX = in["speedX"].asDouble();
        this->speedY = in["speedY"].asDouble();
        this->longitude = in["longitude"].asDouble();
        this->latitude = in["latitude"].asDouble();
        this->laneCode = in["laneCode"].asString();
        this->carLength = in["carLength"].asDouble();
        this->carFeaturePic = in["carFeaturePic"].asString();

        return true;
    }

    bool WatchData::JsonMarshal(Json::Value &out) {

        out["oprNum"] = this->oprNum;
        out["hardCode"] = this->hardCode;
        out["timstamp"] = this->timstamp;
        out["matrixNo"] = this->matrixNo;
        out["cameraIp"] = this->cameraIp;
        out["RecordDateTime"] = this->RecordDateTime;
        out["isHasImage"] = this->isHasImage;
        out["imageData"] = this->imageData;
        out["direction"] = this->direction;

        Json::Value listAnnuciatorInfo = Json::arrayValue;
        if (!this->listAnnuciatorInfo.empty()) {
            for (auto iter:this->listAnnuciatorInfo) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    listAnnuciatorInfo.append(item);
                }
            }
        } else {
            listAnnuciatorInfo.resize(0);
        }
        out["listAnnuciatorInfo"] = listAnnuciatorInfo;

        Json::Value lstObjTarget = Json::arrayValue;
        if (!this->lstObjTarget.empty()) {
            for (auto iter:this->lstObjTarget) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstObjTarget.append(item);
                }
            }
        } else {
            lstObjTarget.resize(0);
        }
        out["lstObjTarget"] = lstObjTarget;

        return true;
    }

    bool WatchData::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->hardCode = in["hardCode"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->matrixNo = in["matrixNo"].asString();
        this->cameraIp = in["cameraIp"].asString();
        this->RecordDateTime = in["RecordDateTime"].asDouble();
        this->isHasImage = in["isHasImage"].asInt();
        this->imageData = in["imageData"].asString();
        this->direction = in["direction"].asInt();

        Json::Value listAnnuciatorInfo = in["listAnnuciatorInfo"];
        if (listAnnuciatorInfo.isArray()) {
            for (auto iter:listAnnuciatorInfo) {
                AnnuciatorInfo item;
                if (item.JsonUnmarshal(iter)) {
                    this->listAnnuciatorInfo.push_back(item);
                }
            }
        }

        Json::Value lstObjTarget = in["lstObjTarget"];
        if (lstObjTarget.isArray()) {
            for (auto iter:lstObjTarget) {
                ObjTarget item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstObjTarget.push_back(item);
                }
            }
        }

        return true;
    }

    bool OneRvWayObject::JsonMarshal(Json::Value &out) {
        out["wayNo"] = this->wayNo;
        out["roID"] = this->roID;
        out["voID"] = this->voID;
        return true;
    }

    bool OneRvWayObject::JsonUnmarshal(Json::Value in) {
        this->wayNo = in["wayNo"].asInt();
        this->roID = in["roID"].asInt();
        this->voID = in["voID"].asInt();
        return true;
    }

    bool VideoTargets::JsonMarshal(Json::Value &out) {
        out["cameraObjID"] = this->cameraObjID;
        out["left"] = this->left;
        out["top"] = this->top;
        out["right"] = this->right;
        out["bottom"] = this->bottom;
        return true;
    }

    bool VideoTargets::JsonUnmarshal(Json::Value in) {
        this->cameraObjID = in["cameraObjID"].asInt();
        this->left = in["left"].asInt();
        this->top = in["top"].asInt();
        this->right = in["right"].asInt();
        this->bottom = in["bottom"].asInt();
        return true;
    }

    bool VideoData::JsonMarshal(Json::Value &out) {
        //rvHardCode
        out["rvHardCode"] = this->rvHardCode;
        //imageData
        out["imageData"] = this->imageData;

        Json::Value lstVideoTargets = Json::arrayValue;
        if (!this->lstVideoTargets.empty()) {
            for (auto iter:this->lstVideoTargets) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstVideoTargets.append(item);
                }
            }
        } else {
            lstVideoTargets.resize(0);
        }
        out["lstVideoTargets"] = lstVideoTargets;

        return true;
    }

    bool VideoData::JsonUnmarshal(Json::Value in) {
        this->rvHardCode = in["rvHardCode"].asString();
        this->imageData = in["imageData"].asString();

        Json::Value lstVideoTargets = in["lstVideoTargets"];
        if (lstVideoTargets.isArray()) {
            for (auto iter:lstVideoTargets) {
                VideoTargets item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstVideoTargets.push_back(item);
                }
            }
        }
        return true;
    }

    bool ObjMix::JsonMarshal(Json::Value &out) {
        out["objID"] = this->objID;
        out["objType"] = this->objType;
        out["objColor"] = this->objColor;
        out["carType"] = this->carType;
        out["plates"] = this->plates;
        out["plateColor"] = this->plateColor;
        out["distance"] = this->distance;
        out["angle"] = this->angle;
        out["speed"] = this->speed;
        out["locationX"] = this->locationX;
        out["locationY"] = this->locationY;
        out["longitude"] = this->longitude;
        out["latitude"] = this->latitude;
        out["laneCode"] = this->laneCode;
        out["carLength"] = this->carLength;
        out["carFeaturePic"] = this->carFeaturePic;
        out["flagNew"] = this->flagNew;

        Json::Value rvWayObject = Json::arrayValue;
        if (!this->rvWayObject.empty()) {
            for (auto iter:this->rvWayObject) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    rvWayObject.append(item);
                }
            }
        } else {
            rvWayObject.resize(0);
        }
        out["rvWayObject"] = rvWayObject;

        return true;
    }

    bool ObjMix::JsonUnmarshal(Json::Value in) {
        this->objID = in["objID"].asInt();
        this->objType = in["objType"].asInt();
        this->objColor = in["objColor"].asInt();
        this->carType = in["catType"].asInt();
        this->plates = in["plates"].asString();
        this->plateColor = in["plateColor"].asString();
        this->distance = in["distance"].asFloat();
        this->angle = in["angle"].asFloat();
        this->speed = in["speed"].asFloat();
        this->locationX = in["locationX"].asDouble();
        this->locationY = in["locationY"].asDouble();
        this->longitude = in["longitude"].asDouble();
        this->latitude = in["latitude"].asDouble();
        this->laneCode = in["laneCode"].asString();
        this->carLength = in["carLength"].asDouble();
        this->carFeaturePic = in["carFeaturePic"].asString();

        this->flagNew = in["flagNew"].asInt();

        Json::Value rvWayObject = in["rvWayObject"];
        if (rvWayObject.isArray()) {
            for (auto iter:rvWayObject) {
                OneRvWayObject item;
                if (item.JsonUnmarshal(iter)) {
                    this->rvWayObject.push_back(item);
                }
            }
        }

        return true;
    }

    bool FusionData::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timstamp"] = this->timstamp;
        out["crossID"] = this->crossID;
        out["isHasImage"] = this->isHasImage;

        Json::Value lstObjTarget = Json::arrayValue;
        if (!this->lstObjTarget.empty()) {
            for (auto iter:this->lstObjTarget) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstObjTarget.append(item);
                }
            }
        } else {
            lstObjTarget.resize(0);
        }
        out["lstObjTarget"] = lstObjTarget;

        Json::Value lstVideos = Json::arrayValue;
        if (!this->lstVideos.empty()) {
            for (auto iter:this->lstVideos) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstVideos.append(item);
                }
            }
        } else {
            lstVideos.resize(0);
        }
        out["lstVideos"] = lstVideos;

        return true;
    }

    bool FusionData::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->isHasImage = in["isHasImage"].asInt();

        Json::Value lstObjTarget = in["lstObjTarget"];
        if (lstObjTarget.isArray()) {
            for (auto iter:lstObjTarget) {
                ObjMix item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstObjTarget.push_back(item);
                }
            }
        }

        Json::Value lstVideos = in["lstVideos"];
        if (lstVideos.isArray()) {
            for (auto iter:lstVideos) {
                VideoData item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstVideos.push_back(item);
                }
            }
        }

        return true;
    }

//    int PkgFusionDataWithoutCRC(FusionData fusionData, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
//        int len = 0;
//        //1.头部
//        pkg.head.tag = '$';
//        pkg.head.version = 1;
//        pkg.head.cmd = CmdType::CmdFusionData;
//        pkg.head.sn = sn;
//        pkg.head.deviceNO = deviceNO;
//        pkg.head.len = 0;
//        len += sizeof(pkg.head);
//        //正文
//        string jsonStr;
//        Json::FastWriter fastWriter;
//        Json::Value root;
//        fusionData.JsonMarshal(root);
//        jsonStr = fastWriter.write(root);
//        pkg.body = jsonStr;
//        len += jsonStr.length();
//        //校验,可以先不设置，等待组包的时候更新
//        pkg.crc.data = 0x0000;
//        len += sizeof(pkg.crc);
//
//        pkg.head.len = len;
//
//        return 0;
//    }

    bool OneFlowData::JsonMarshal(Json::Value &out) {
        out["laneCode"] = this->laneCode;
        out["laneDirection"] = this->laneDirection;
        out["flowDirection"] = this->flowDirection;
        out["inCars"] = this->inCars;
        out["inAverageSpeed"] = this->inAverageSpeed;
        out["outCars"] = this->outCars;
        out["outAverageSpeed"] = this->outAverageSpeed;
        out["queueLen"] = this->queueLen;
        out["queueCars"] = this->queueCars;
        return true;
    }

    bool OneFlowData::JsonUnmarshal(Json::Value in) {
        this->laneCode = in["laneCode"].asString();
        this->laneDirection = in["laneDirection"].asInt();
        this->flowDirection = in["flowDirection"].asInt();
        this->inCars = in["inCars"].asInt();
        this->inAverageSpeed = in["inAverageSpeed"].asDouble();
        this->outCars = in["outCars"].asInt();
        this->outAverageSpeed = in["outAverageSpeed"].asDouble();
        this->queueLen = in["queueLen"].asInt();
        this->queueCars = in["queueCars"].asInt();
        return true;
    }

    bool TrafficFlow::JsonMarshal(Json::Value &out) {

        out["oprNum"] = this->oprNum;
        out["hardCode"] = this->hardCode;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;

        Json::Value flowData = Json::arrayValue;
        if (!this->flowData.empty()) {
            for (auto iter:this->flowData) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    flowData.append(item);
                }
            }
        } else {
            flowData.resize(0);
        }
        out["flowData"] = flowData;

        return true;
    }

    bool TrafficFlow::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->hardCode = in["hardCode"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();

        if (in["flowData"].isArray()) {
            Json::Value flowData = in["flowData"];
            for (auto iter:flowData) {
                OneFlowData item;
                if (item.JsonUnmarshal(iter)) {
                    this->flowData.push_back(item);
                }
            }
        }

        return true;
    }

    bool TrafficFlowGather::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["recordDateTime"] = this->recordDateTime;

        Json::Value trafficFlow = Json::arrayValue;
        if (!this->trafficFlow.empty()) {
            for (auto iter:this->trafficFlow) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    trafficFlow.append(item);
                }
            }
        } else {
            trafficFlow.resize(0);
        }
        out["trafficFlow"] = trafficFlow;

        return true;
    }

    bool TrafficFlowGather::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->crossID = in["crossID"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->recordDateTime = in["recordDateTime"].asString();

        if (in["trafficFlow"].isArray()) {
            Json::Value trafficFlow = in["trafficFlow"];
            for (auto iter:trafficFlow) {
                OneFlowData item;
                if (item.JsonUnmarshal(iter)) {
                    this->trafficFlow.push_back(item);
                }
            }
        }

        return true;
    }

//    int PkgTrafficFlowGatherWithoutCRC(TrafficFlowGather trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
//        int len = 0;
//        //1.头部
//        pkg.head.tag = '$';
//        pkg.head.version = 1;
//        pkg.head.cmd = CmdType::CmdTrafficFlowGather;
//        pkg.head.sn = sn;
//        pkg.head.deviceNO = deviceNO;
//        pkg.head.len = 0;
//        len += sizeof(pkg.head);
//        //正文
//        string jsonStr;
//        Json::FastWriter fastWriter;
//        Json::Value root;
//        trafficFlows.JsonMarshal(root);
//        jsonStr = fastWriter.write(root);
//        pkg.body = jsonStr;
//        len += jsonStr.length();
//        //校验,可以先不设置，等待组包的时候更新
//        pkg.crc.data = 0x0000;
//        len += sizeof(pkg.crc);
//
//        pkg.head.len = len;
//
//        return 0;
//    }

    bool CrossTrafficJamAlarm::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;
//        out["roadDirection"] = this->roadDirection;
        out["alarmType"] = this->alarmType;
        out["alarmStatus"] = this->alarmStatus;
        out["alarmTime"] = this->alarmTime;

        return true;
    }

    bool CrossTrafficJamAlarm::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();
//        this->roadDirection = in["roadDirection"].asInt();
        this->alarmType = in["alarmType"].asInt();
        this->alarmStatus = in["alarmStatus"].asInt();
        this->alarmTime = in["alarmTime"].asString();

        return true;
    }

//    int PkgCrossTrafficJamAlarmWithoutCRC(CrossTrafficJamAlarm crossTrafficJamAlarm, uint16_t sn, uint32_t deviceNO,
//                                          Pkg &pkg) {
//        int len = 0;
//        //1.头部
//        pkg.head.tag = '$';
//        pkg.head.version = 1;
//        pkg.head.cmd = CmdType::CmdCrossTrafficJamAlarm;
//        pkg.head.sn = sn;
//        pkg.head.deviceNO = deviceNO;
//        pkg.head.len = 0;
//        len += sizeof(pkg.head);
//        //正文
//        string jsonStr;
//        Json::FastWriter fastWriter;
//        Json::Value root;
//        crossTrafficJamAlarm.JsonMarshal(root);
//        jsonStr = fastWriter.write(root);
//
//        pkg.body = jsonStr;
//        len += jsonStr.length();
//        //校验,可以先不设置，等待组包的时候更新
//        pkg.crc.data = 0x0000;
//        len += sizeof(pkg.crc);
//
//        pkg.head.len = len;
//
//        return 0;
//    }

    bool IntersectionOverflowAlarm::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;
        out["roadDirection"] = this->roadDirection;
        out["alarmType"] = this->alarmType;
        out["alarmStatus"] = this->alarmStatus;
        out["alarmTime"] = this->alarmTime;

        return true;
    }

    bool IntersectionOverflowAlarm::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();
        this->roadDirection = in["roadDirection"].asInt();
        this->alarmType = in["alarmType"].asInt();
        this->alarmStatus = in["alarmStatus"].asInt();
        this->alarmTime = in["alarmTime"].asString();

        return true;
    }

//    int PkgIntersectionOverflowAlarmWithoutCRC(IntersectionOverflowAlarm intersectionOverflowAlarm, uint16_t sn,
//                                               uint32_t deviceNO, Pkg &pkg) {
//        int len = 0;
//        //1.头部
//        pkg.head.tag = '$';
//        pkg.head.version = 1;
//        pkg.head.cmd = CmdType::CmdIntersectionOverflowAlarm;
//        pkg.head.sn = sn;
//        pkg.head.deviceNO = deviceNO;
//        pkg.head.len = 0;
//        len += sizeof(pkg.head);
//        //正文
//        string jsonStr;
//        Json::FastWriter fastWriter;
//        Json::Value root;
//        intersectionOverflowAlarm.JsonMarshal(root);
//        jsonStr = fastWriter.write(root);
//
//        pkg.body = jsonStr;
//        len += jsonStr.length();
//        //校验,可以先不设置，等待组包的时候更新
//        pkg.crc.data = 0x0000;
//        len += sizeof(pkg.crc);
//
//        pkg.head.len = len;
//
//        return 0;
//    }

    bool InWatchData_1_3_4::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;
        out["laneCode"] = this->laneCode;
        out["laneDirection"] = this->laneDirection;
        out["flowDirection"] = this->flowDirection;
        out["detectLocation"] = this->detectLocation;
        out["vehicleID"] = this->vehicleID;
        out["vehicleType"] = this->vehicleType;
        out["vehicleSpeed"] = this->vehicleSpeed;
        return true;
    }

    bool InWatchData_1_3_4::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();
        this->laneCode = in["laneCode"].asString();
        this->laneDirection = in["laneDirection"].asInt();
        this->flowDirection = in["flowDirection"].asInt();
        this->detectLocation = in["detectLocation"].asInt();
        this->vehicleID = in["vehicleID"].asInt();
        this->vehicleType = in["vehicleType"].asInt();
        this->vehicleSpeed = in["vehicleSpeed"].asInt();
        return true;
    }

//    int PkgInWatchData_1_3_4WithoutCRC(InWatchData_1_3_4 inWatchData134, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
//        int len = 0;
//        //1.头部
//        pkg.head.tag = '$';
//        pkg.head.version = 1;
//        pkg.head.cmd = CmdType::CmdInWatchData_1_3_4;
//        pkg.head.sn = sn;
//        pkg.head.deviceNO = deviceNO;
//        pkg.head.len = 0;
//        len += sizeof(pkg.head);
//        //正文
//        string jsonStr;
//        Json::FastWriter fastWriter;
//        Json::Value root;
//        inWatchData134.JsonMarshal(root);
//        jsonStr = fastWriter.write(root);
//
//        pkg.body = jsonStr;
//        len += jsonStr.length();
//        //校验,可以先不设置，等待组包的时候更新
//        pkg.crc.data = 0x0000;
//        len += sizeof(pkg.crc);
//
//        pkg.head.len = len;
//
//        return 0;
//    }

    bool InWatchData_2_trafficFlowListItem_vehicleIDListItem::JsonMarshal(Json::Value &out) {
        out["vehicleID"] = this->vehicleID;
        out["vehicleType"] = this->vehicleType;
        out["vehicleSpeed"] = this->vehicleSpeed;
        return true;
    }

    bool InWatchData_2_trafficFlowListItem_vehicleIDListItem::JsonUnmarshal(Json::Value in) {
        this->vehicleID = in["vehicleID"].asInt();
        this->vehicleType = in["vehicleType"].asInt();
        this->vehicleSpeed = in["vehicleSpeed"].asInt();
        return true;
    }

    bool InWatchData_2_trafficFlowListItem::JsonMarshal(Json::Value &out) {
        out["laneCode"] = this->laneCode;
        out["laneDirection"] = this->laneDirection;
        out["flowDirection"] = this->flowDirection;

        Json::Value vehicleIDList = Json::arrayValue;
        if (!this->vehicleIDList.empty()) {
            for (auto iter:this->vehicleIDList) {
                Json::Value item;
                iter.JsonMarshal(item);
                vehicleIDList.append(item);
            }
        } else {
            vehicleIDList.resize(0);
        }

        out["vehicleIDList"] = vehicleIDList;

        return true;
    }

    bool InWatchData_2_trafficFlowListItem::JsonUnmarshal(Json::Value in) {
        this->laneCode = in["laneCode"].asString();
        this->laneDirection = in["laneDirection"].asInt();
        this->flowDirection = in["flowDirection"].asInt();

        Json::Value vehicleIDList = in["vehicleIDList"];
        if (vehicleIDList.isArray()) {
            for (auto iter:vehicleIDList) {
                InWatchData_2_trafficFlowListItem_vehicleIDListItem item;
                if (item.JsonUnmarshal(iter)) {
                    this->vehicleIDList.push_back(item);
                }
            }
        } else {
            return false;
        }

        return true;
    }

    bool InWatchData_2::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;
        out["recordLaneSum"] = this->recordLaneSum;

        Json::Value trafficFlowList = Json::arrayValue;
        if (!this->trafficFlowList.empty()) {
            for (auto iter:this->trafficFlowList) {
                Json::Value item;
                iter.JsonMarshal(item);
                trafficFlowList.append(item);
            }
        } else {
            trafficFlowList.resize(0);
        }

        out["trafficFlowList"] = trafficFlowList;


        return true;
    }

    bool InWatchData_2::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();
        this->recordLaneSum = in["recordLaneSum"].asInt();

        Json::Value trafficFlowList = in["trafficFlowList"];
        if (trafficFlowList.isArray()) {
            for (auto iter:trafficFlowList) {
                InWatchData_2_trafficFlowListItem item;
                if (item.JsonUnmarshal(iter)) {
                    this->trafficFlowList.push_back(item);
                }
            }
        } else {
            return false;
        }

        return true;
    }

    bool StopLinePassData_vehicleListItem::JsonMarshal(Json::Value &out) {
        out["laneCode"] = this->laneCode;
        out["laneDirection"] = this->laneDirection;
        out["flowDirection"] = this->flowDirection;
        out["vehiclePlate"] = this->vehiclePlate;
        out["vehiclePlateColor"] = this->vehiclePlateColor;
        out["vehicleType"] = this->vehicleType;
        out["vehicleSpeed"] = this->vehicleSpeed;

        return true;
    }

    bool StopLinePassData_vehicleListItem::JsonUnmarshal(Json::Value in) {
        this->laneCode = in["laneCode"].asString();
        this->laneDirection = in["laneDirection"].asInt();
        this->flowDirection = in["flowDirection"].asInt();
        this->vehiclePlate = in["vehiclePlate"].asString();
        this->vehiclePlateColor = in["vehiclePlateColor"].asString();
        this->vehicleType = in["vehicleType"].asInt();
        this->vehicleSpeed = in["vehicleSpeed"].asInt();

        return true;
    }

    bool StopLinePassData::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;

        Json::Value vehicleList = Json::arrayValue;
        if (!this->vehicleList.empty()) {
            for (auto iter:this->vehicleList) {
                Json::Value item;
                iter.JsonMarshal(item);
                vehicleList.append(item);
            }
        } else {
            vehicleList.resize(0);
        }

        out["vehicleList"] = vehicleList;


        return true;
    }

    bool StopLinePassData::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();

        Json::Value vehicleList = in["vehicleList"];
        if (vehicleList.isArray()) {
            for (auto iter:vehicleList) {
                StopLinePassData_vehicleListItem item;
                if (item.JsonUnmarshal(iter)) {
                    this->vehicleList.push_back(item);
                }
            }
        } else {
            return false;
        }

        return true;
    }

    bool Camera3516Alarm_areaListItem::JsonMarshal(Json::Value &out) {
        out["p0x"] = this->p0x;
        out["p0y"] = this->p0y;
        out["p1x"] = this->p1x;
        out["p1y"] = this->p1y;
        out["p2x"] = this->p2x;
        out["p2y"] = this->p2y;
        out["p3x"] = this->p3x;
        out["p3y"] = this->p3y;
        out["humanNum"] = this->humanNum;
        out["humanType"] = this->humanType;
        out["bicycleNum"] = this->bicycleNum;

        return true;
    }

    bool Camera3516Alarm_areaListItem::JsonUnmarshal(Json::Value in) {
        this->p0x = in["p0x"].asInt();
        this->p0y = in["p0y"].asInt();
        this->p1x = in["p1x"].asInt();
        this->p1y = in["p1y"].asInt();
        this->p2x = in["p2x"].asInt();
        this->p2y = in["p2y"].asInt();
        this->p3x = in["p3x"].asInt();
        this->p3y = in["p3y"].asInt();
        this->humanNum = in["humanNum"].asInt();
        this->humanType = in["humanType"].asInt();
        this->bicycleNum = in["bicycleNum"].asInt();
        return true;
    }

    bool Camera3516Alarm::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["hardCode"] = this->hardCode;

        Json::Value areaList = Json::arrayValue;
        if (!this->areaList.empty()) {
            for (auto iter:this->areaList) {
                Json::Value item;
                iter.JsonMarshal(item);
                areaList.append(item);
            }
        } else {
            areaList.resize(0);
        }

        out["areaList"] = areaList;
        return true;
    }

    bool Camera3516Alarm::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->hardCode = in["hardCode"].asString();

        Json::Value areaList = in["areaList"];
        if (areaList.isArray()) {
            for (auto iter:areaList) {
                Camera3516Alarm_areaListItem item;
                if (item.JsonUnmarshal(iter)) {
                    this->areaList.push_back(item);
                }
            }
        } else {
            return false;
        }

        return true;
    }
}

