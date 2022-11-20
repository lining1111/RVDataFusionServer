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

        return true;
    }

    bool ObjTarget::JsonUnmarshal(Json::Value in) {
        this->objID = in["objID"].asInt();
        this->objCameraID = in["objCameraID"].asInt();
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

    bool RvWayObject::JsonMarshal(Json::Value &out) {
        out["wayNo"] = this->wayNo;
        out["roID"] = this->roID;
        out["voID"] = this->voID;
        return true;
    }

    bool RvWayObject::JsonUnmarshal(Json::Value in) {
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
        out["plates"] = this->plates;
        out["plateColor"] = this->plateColor;
        out["distance"] = this->distance;
        out["angle"] = this->angle;
        out["speed"] = this->speed;
        out["locationX"] = this->locationX;
        out["locationY"] = this->locationY;
        out["longitude"] = this->longitude;
        out["latitude"] = this->latitude;
        out["flagNew"] = this->flagNew;

        Json::Value listRvWayObject = Json::arrayValue;
        if (!this->listRvWayObject.empty()) {
            for (auto iter:this->listRvWayObject) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    listRvWayObject.append(item);
                }
            }
        } else {
            listRvWayObject.resize(0);
        }
        out["listRvWayObject"] = listRvWayObject;

        return false;
    }

    bool ObjMix::JsonUnmarshal(Json::Value in) {
        this->objID = in["objID"].asInt();
        this->objType = in["objType"].asInt();
        this->objColor = in["objColor"].asInt();
        this->plates = in["plates"].asString();
        this->plateColor = in["plateColor"].asString();
        this->distance = in["distance"].asFloat();
        this->angle = in["angle"].asFloat();
        this->speed = in["speed"].asFloat();
        this->locationX = in["locationX"].asDouble();
        this->locationY = in["locationY"].asDouble();
        this->longitude = in["longitude"].asDouble();
        this->latitude = in["latitude"].asDouble();
        this->flagNew = in["flagNew"].asInt();

        Json::Value listRvWayObject = in["listRvWayObject"];
        if (listRvWayObject.isArray()) {
            for (auto iter:listRvWayObject) {
                RvWayObject item;
                if (item.JsonUnmarshal(iter)) {
                    this->listRvWayObject.push_back(item);
                }
            }
        }

        return false;
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
        Json::FastWriter fastWriter;
        Json::Value root;
        fusionData.JsonMarshal(root);
        jsonStr = fastWriter.write(root);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }

    bool FlowData::JsonMarshal(Json::Value &out) {
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

    bool FlowData::JsonUnmarshal(Json::Value in) {
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
        out["timstamp"] = this->timstamp;
        out["crossCode"] = this->crossCode;

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
        this->timstamp = in["timstamp"].asDouble();
        this->crossCode = in["crossCode"].asString();

        if (in["flowData"].isArray()) {
            Json::Value flowData = in["flowData"];
            for (auto iter:flowData) {
                FlowData item;
                if (item.JsonUnmarshal(iter)) {
                    this->flowData.push_back(item);
                }
            }
        }

        return true;
    }

    bool TrafficFlows::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timstamp"] = this->timestamp;
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

    bool TrafficFlows::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->crossID = in["crossID"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->recordDateTime = in["recordDateTime"].asString();

        if (in["trafficFlow"].isArray()) {
            Json::Value trafficFlow = in["trafficFlow"];
            for (auto iter:trafficFlow) {
                FlowData item;
                if (item.JsonUnmarshal(iter)) {
                    this->trafficFlow.push_back(item);
                }
            }
        }

        return true;
    }

    int PkgTrafficFlowsWithoutCRC(TrafficFlows trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceMultiview;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        Json::FastWriter fastWriter;
        Json::Value root;
        trafficFlows.JsonMarshal(root);
        jsonStr = fastWriter.write(root);
        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }

    bool CarTrack::JsonMarshal(Json::Value &out) {
        out["id"] = this->id;
        out["type"] = this->type;
        out["cameraDirection"] = this->cameraDirection;
        out["x1"] = this->x1;
        out["y1"] = this->y1;
        out["x2"] = this->x2;
        out["y2"] = this->y2;
        out["latitude"] = this->latitude;
        out["longitude"] = this->longitude;
        out["laneCode"] = this->laneCode;
        out["speed"] = this->speed;
        out["timeHeadway"] = this->timeHeadway;
        out["plateNumber"] = this->plateNumber;
        out["plateColor"] = this->plateColor;
        return true;
    }

    bool CarTrack::JsonUnmarshal(Json::Value in) {
        this->id = in["id"].asInt();
        this->type = in["type"].asInt();
        this->cameraDirection = in["cameraDirection"].asInt();
        this->x1 = in["x1"].asInt();
        this->y1 = in["y1"].asInt();
        this->x2 = in["x2"].asInt();
        this->y2 = in["y2"].asInt();
        this->latitude = in["latitude"].asInt();
        this->longitude = in["longitude"].asInt();
        this->laneCode = in["laneCode"].asString();
        this->speed = in["speed"].asInt();
        this->timeHeadway = in["timeHeadway"].asInt();
        this->plateNumber = in["plateNumber"].asString();
        this->plateColor = in["plateColor"].asString();
        return true;
    }

    bool MultiViewCarTrack::JsonMarshal(Json::Value &out) {

        out["oprNum"] = this->oprNum;
        out["hardCode"] = this->hardCode;
        out["timstamp"] = this->timstamp;
        out["crossCode"] = this->crossCode;
        out["ip"] = this->ip;
        out["type"] = this->type;

        Json::Value lstObj = Json::arrayValue;
        if (!this->lstObj.empty()) {
            for (auto iter:this->lstObj) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstObj.append(item);
                }
            }
        } else {
            lstObj.resize(0);
        }

        out["lstObj"] = lstObj;

        return true;
    }

    bool MultiViewCarTrack::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->hardCode = in["hardCode"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->crossCode = in["crossCode"].asString();
        this->ip = in["ip"].asString();
        this->type = in["type"].asInt();

        if (in["lstObj"].isArray()) {
            Json::Value lstObj = in["lstObj"];
            for (auto iter:lstObj) {
                CarTrack item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstObj.push_back(item);
                }
            }
        }
        return true;
    }

    bool CrossTrafficJamAlarm::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timstamp"] = this->timstamp;
        out["crossCode"] = this->crossCode;
        out["hardCode"] = this->hardCode;
        out["alarmType"] = this->alarmType;
        out["alarmStatus"] = this->alarmStatus;
        out["alarmTime"] = this->alarmTime;

        return true;
    }

    bool CrossTrafficJamAlarm::JsonUnmarshal(Json::Value in) {

        this->oprNum = in["oprNum"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->crossCode = in["crossCode"].asString();
        this->hardCode = in["hardCode"].asString();
        this->alarmType = in["alarmType"].asInt();
        this->alarmStatus = in["alarmStatus"].asInt();
        this->alarmTime = in["alarmTime"].asInt();

        return true;
    }

    int PkgCrossTrafficJamAlarmWithoutCRC(CrossTrafficJamAlarm crossTrafficJamAlarm, uint16_t sn, uint32_t deviceNO,
                                          Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceAlarm;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        Json::FastWriter fastWriter;
        Json::Value root;
        crossTrafficJamAlarm.JsonMarshal(root);
        jsonStr = fastWriter.write(root);

        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }

    bool TrafficFlowLineup::JsonMarshal(Json::Value &out) {

        out["laneID"] = this->laneID;
        out["averageSpeed"] = this->averageSpeed;
        out["flow"] = this->flow;
        out["queueLength"] = this->queueLength;
        out["sumMini"] = this->sumMini;
        out["sumMid"] = this->sumMid;
        out["sumMax"] = this->sumMax;
        out["headWay"] = this->headWay;
        out["headWayTime"] = this->headWayTime;
        out["occupancy"] = this->occupancy;
        out["occupancySpace"] = this->occupancySpace;

        return true;
    }

    bool TrafficFlowLineup::JsonUnmarshal(Json::Value in) {
        this->laneID = in["laneID"].asInt();
        this->averageSpeed = in["averageSpeed"].asInt();
        this->flow = in["flow"].asInt();
        this->queueLength = in["queueLength"].asInt();
        this->sumMini = in["sumMini"].asInt();
        this->sumMid = in["sumMid"].asInt();
        this->sumMax = in["sumMax"].asInt();
        this->headWay = in["headWay"].asInt();
        this->headWayTime = in["headWayTime"].asInt();
        this->occupancy = in["occupancy"].asInt();
        this->occupancySpace = in["occupancySpace"].asInt();

        return true;
    }

    bool LineupInfo::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timstamp"] = this->timstamp;
        out["crossCode"] = this->crossCode;
        out["hardCode"] = this->hardCode;
        out["recordDateTime"] = this->recordDateTime;

        Json::Value trafficFlowList = Json::arrayValue;
        if (!this->trafficFlowList.empty()) {
            for (auto iter:this->trafficFlowList) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    trafficFlowList.append(item);
                }
            }
        } else {
            trafficFlowList.resize(0);
        }

        out["trafficFlowList"] = trafficFlowList;

        return true;
    }

    bool LineupInfo::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->crossCode = in["crossCode"].asString();
        this->hardCode = in["hardCode"].asString();
        this->recordDateTime = in["recordDateTime"].asString();

        Json::Value trafficFlowList = in["trafficFlowList"];
        if (trafficFlowList.isArray()) {
            for (auto iter:trafficFlowList) {
                TrafficFlowLineup item;
                if (item.JsonUnmarshal(iter)) {
                    this->trafficFlowList.push_back(item);
                }
            }
        }

        return true;
    }

    bool LineupInfoGather::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timstamp"] = this->timstamp;
        out["crossCode"] = this->crossCode;
        out["hardCode"] = this->hardCode;

        Json::Value trafficFlowList = Json::arrayValue;
        if (!this->trafficFlowList.empty()) {
            for (auto iter:this->trafficFlowList) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    trafficFlowList.append(item);
                }
            }
        } else {
            trafficFlowList.resize(0);
        }

        out["TrafficFlowList"] = trafficFlowList;

        return true;
    }

    bool LineupInfoGather::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timstamp = in["timstamp"].asDouble();
        this->crossCode = in["crossCode"].asString();
        this->hardCode = in["hardCode"].asString();

        Json::Value trafficFlowList = in["trafficFlowList"];
        if (trafficFlowList.isArray()) {
            for (auto iter:trafficFlowList) {
                TrafficFlowLineup item;
                if (item.JsonUnmarshal(iter)) {
                    this->trafficFlowList.push_back(item);
                }
            }
        }

        return true;
    }

    int PkgLineupInfoGatherWithoutCRC(LineupInfoGather lineupInfoGather, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceStatus;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        Json::FastWriter fastWriter;
        Json::Value root;
        lineupInfoGather.JsonMarshal(root);
        jsonStr = fastWriter.write(root);

        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }



    bool MultiViewCarTracks::JsonMarshal(Json::Value &out) {
        out["oprNum"] = this->oprNum;
        out["timestamp"] = this->timestamp;
        out["crossID"] = this->crossID;
        out["recordDateTime"] = this->recordDateTime;

        Json::Value lstObj = Json::arrayValue;
        if (!this->lstObj.empty()) {
            for (auto iter:this->lstObj) {
                Json::Value item;
                if (iter.JsonMarshal(item)) {
                    lstObj.append(item);
                }
            }
        } else {
            lstObj.resize(0);
        }

        out["lstObj"] = lstObj;

        return true;
    }

    bool MultiViewCarTracks::JsonUnmarshal(Json::Value in) {
        this->oprNum = in["oprNum"].asString();
        this->timestamp = in["timestamp"].asDouble();
        this->crossID = in["crossID"].asString();
        this->recordDateTime = in["recordDateTime"].asString();

        Json::Value lstObj = in["lstObj"];
        if (lstObj.isArray()) {
            for (auto iter:lstObj) {
                CarTrack item;
                if (item.JsonUnmarshal(iter)) {
                    this->lstObj.push_back(item);
                }
            }
        }
        return true;
    }
    int
    PkgMultiViewCarTracksWithoutCRC(MultiViewCarTracks multiViewCarTracks, uint16_t sn, uint32_t deviceNO, Pkg &pkg) {
        int len = 0;
        //1.头部
        pkg.head.tag = '$';
        pkg.head.version = 1;
        pkg.head.cmd = CmdType::DeviceMultiviewCarTrack;
        pkg.head.sn = sn;
        pkg.head.deviceNO = deviceNO;
        pkg.head.len = 0;
        len += sizeof(pkg.head);
        //正文
        string jsonStr;
        Json::FastWriter fastWriter;
        Json::Value root;
        multiViewCarTracks.JsonMarshal(root);
        jsonStr = fastWriter.write(root);

        pkg.body = jsonStr;
        len += jsonStr.length();
        //校验,可以先不设置，等待组包的时候更新
        pkg.crc.data = 0x0000;
        len += sizeof(pkg.crc);

        pkg.head.len = len;

        return 0;
    }
}

