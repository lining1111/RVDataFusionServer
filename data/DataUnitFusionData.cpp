//
// Created by lining on 2023/5/27.
//

#include "DataUnitFusionData.h"
#include "Data.h"
#include <glog/logging.h>
#include <future>
#include <math.h>
#include <string>
#include <fstream>
#include <iomanip>
#include "common/config.h"
#include "DataUnitUtilty.h"
#include "localBussiness/localBusiness.h"

using namespace std;

bool isProcessMerge = false;//task是否执行了融合流程
void DataUnitFusionData::taskI() {

    uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    isProcessMerge = false;
    auto data = (Data *) this->owner;

    DataUnitFusionData::MergeType mergeType;
    if (data->isMerge) {
        mergeType = DataUnitFusionData::Merge;
    } else {
        mergeType = DataUnitFusionData::NotMerge;
    }

    this->runTask(std::bind(DataUnitFusionData::FindOneFrame, this, mergeType, this->cache / 2));
    if (isProcessMerge) {
        uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        totalCost = timestampEnd - timestampStart;
        LOG(INFO) << "DataUnitFusionData 取同一帧完成融合耗时" << totalCost << "ms";
    }
}

void DataUnitFusionData::FindOneFrame(DataUnitFusionData *dataUnit, MergeType mergeType, int offset) {
    if (DataUnit::FindOneFrame(dataUnit, offset) == 0) {
        //调用后续的处理
        dataUnit->TaskProcessOneFrame(mergeType);
    }
}

int DataUnitFusionData::TaskProcessOneFrame(DataUnitFusionData::MergeType mergeType) {
    //1，将同一帧内待输入量存入集合
    DataUnitFusionData::RoadDataInSet roadDataInSet;
    roadDataInSet.roadDataList.resize(numI);
    for (auto &iter: roadDataInSet.roadDataList) {
        iter.imageData.clear();
        iter.listObjs.clear();
        iter.hardCode.clear();
        iter.direction = -1;
        iter.timestamp = 0;
    }


    roadDataInSet.timestamp = curTimestamp;
    for (int i = 0; i < oneFrame.size(); i++) {
        auto iter = oneFrame.at(i);

        DataUnitFusionData::RoadData item;
        item.hardCode = iter.hardCode;
        item.imageData = iter.imageData;
        item.direction = iter.direction;
        item.timestamp = iter.timestamp;
        for (int j = 0; j < iter.lstObjTarget.size(); j++) {
            auto iter1 = iter.lstObjTarget.at(j);
            OBJECT_INFO_T objectInfoT;
            //转换数据类型
            ObjTarget2OBJECT_INFO_T(iter1, objectInfoT);
            item.listObjs.push_back(objectInfoT);
        }
        //存入对应路的集合
        roadDataInSet.roadDataList[i] = item;
    }
    int ret = 0;
    switch (mergeType) {
        case DataUnitFusionData::NotMerge: {
            auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            ret = TaskNotMerge(roadDataInSet);
            auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            algorithmCost = endTime - startTime;
        }
            break;
        case DataUnitFusionData::Merge: {
            auto startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            ret = TaskMerge(roadDataInSet);
            auto endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            algorithmCost = endTime - startTime;
            VLOG(3) << name << " 融合算法耗时:" << algorithmCost << " ms";
        }
            break;
    }
    return ret;
}

int DataUnitFusionData::TaskNotMerge(DataUnitFusionData::RoadDataInSet roadDataInSet) {
    VLOG(3) << "DataUnitFusionData不融合:数据选取的时间戳:" << roadDataInSet.timestamp;
    MergeData mergeData;
    mergeData.timestamp = roadDataInSet.timestamp;
    mergeData.objOutput.clear();
    mergeData.objInput = roadDataInSet;

    const int INF = 0x7FFFFFFF;
    //输出量
    for (int i = 0; i < roadDataInSet.roadDataList.size(); i++) {
        auto iter = roadDataInSet.roadDataList.at(i);
        for (auto iter1: iter.listObjs) {
            OBJECT_INFO_NEW item;
            switch (i) {
                case 0: {
                    //North
                    item.objID1 = iter1.objID;
                    item.cameraID1 = iter1.cameraID;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 10000;
                }
                    break;
                case 1: {
                    //East
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = iter1.objID;
                    item.cameraID2 = iter1.cameraID;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 20000;
                }
                    break;
                case 2: {
                    //South
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = iter1.objID;
                    item.cameraID3 = iter1.cameraID;
                    item.objID4 = -INF;
                    item.cameraID4 = -INF;

                    item.showID = iter1.objID + 30000;
                }
                    break;
                case 3: {
                    //West
                    item.objID1 = -INF;
                    item.cameraID1 = -INF;
                    item.objID2 = -INF;
                    item.cameraID2 = -INF;
                    item.objID3 = -INF;
                    item.cameraID3 = -INF;
                    item.objID4 = iter1.objID;
                    item.cameraID4 = iter1.cameraID;

                    item.showID = iter1.objID + 40000;
                }
                    break;
            }
            item.objType = iter1.objType;
            memcpy(item.plate_number, iter1.plate_number, sizeof(iter1.plate_number));
            memcpy(item.plate_color, iter1.plate_color, sizeof(iter1.plate_color));
            item.left = iter1.left;
            item.top = iter1.top;
            item.right = iter1.right;
            item.bottom = iter1.bottom;
            item.locationX = iter1.locationX;
            item.locationY = iter1.locationY;
            memcpy(item.distance, iter1.distance, sizeof(iter1.distance));
            item.directionAngle = iter1.directionAngle;
            item.speed = sqrt(iter1.speedX * iter1.speedX + iter1.speedY * iter1.speedY);
            item.latitude = iter1.latitude;
            item.longitude = iter1.longitude;
            item.flag_new = 0;

            mergeData.objOutput.push_back(item);
        }
    }

    return GetFusionData(mergeData);
}

int DataUnitFusionData::TaskMerge(RoadDataInSet roadDataInSet) {
    VLOG(3) << "DataUnitFusionData融合:数据选取的时间戳:" << roadDataInSet.timestamp;

    //将取同一帧结果按要求存入算法输入量,后续算法部分输入量用MergeData变量
    MergeData mergeData;
    mergeData.timestamp = roadDataInSet.timestamp;
    mergeData.objOutput.clear();
    mergeData.objInput = roadDataInSet;

    //如果只有一路数据，不走融合
    //将标定时间戳，和帧内时间戳输出到文件
    string line = "";
    line += to_string(this->curTimestamp);
    line += ",";
    this->haveDataRoadNum = 0;
    for (int i = 0; i < this->oneFrame.size(); i++) {
        auto iter = this->oneFrame.at(i);
        line += to_string((uint64_t) iter.timestamp);
        line += ",";
        if (!iter.oprNum.empty()) {
            this->haveDataRoadNum++;
        }
    }
    line += "\n";
    //写入文件
    if (0) {
        ofstream ofs;
        ofs.open("/mnt/mnt_hd/timestamp.csv", ios::out | ios::app);
        //判断大小是否超过最大值
        uint64_t maxSize = 1024 * 1024 * 100;
        if (ofs.tellp() >= maxSize) {
            //清空
            ofs.close();
            ofstream ofs1;
            ofs1.open("/mnt/mnt_hd/timestamp.csv", ios::out | ios::trunc);
            ofs1.flush();
            ofs1.close();
            ofs.open("/mnt/mnt_hd/timestamp.csv", ios::out | ios::app);
        }

        if (ofs.is_open()) {
            ofs << line;
            ofs.flush();
            ofs.close();
        }
    }

    //存输入的目标数据元素到文件
    if (localConfig.isSaveInObj) {
        for (int i = 0; i < mergeData.objInput.roadDataList.size(); i++) {
            auto iter = mergeData.objInput.roadDataList.at(i);
            string path = "/mnt/mnt_hd/save/FusionData/In/" + to_string(i) + "/";
            SaveDataIn(iter.listObjs, mergeData.timestamp, path);
        }
    }

    //这里是根据含有识别数据路数来操作输出量
    switch (this->haveDataRoadNum) {
        case 0 : {
            VLOG(3) << "融合算法细节---全部路都没有数据";
        }
            break;
        case 1 : {
            VLOG(3) << "融合算法细节---只有1路有数据,不走融合";
            for (int i = 0; i < mergeData.objInput.roadDataList.size(); i++) {
                auto iter = mergeData.objInput.roadDataList.at(i);
                if (!iter.listObjs.empty()) {
                    //输出量直接取输入量
                    for (auto iter1: iter.listObjs) {
                        OBJECT_INFO_NEW item;
                        OBJECT_INFO_T2OBJECT_INFO_NEW(iter1, item);
                        mergeData.objOutput.push_back(item);
                    }
                }
            }
        }
            break;
        default : {
            VLOG(3) << "融合算法细节---多于1路有数据,走融合";
            OBJECT_INFO_NEW dataOut[1000];
            memset(dataOut, 0, ARRAY_SIZE(dataOut) * sizeof(OBJECT_INFO_NEW));
            bool isFirstFrame = true;//如果算法缓存内有数则为假
            if (!cacheAlgorithmMerge.empty()) {
                isFirstFrame = false;
            }
            vector<OBJECT_INFO_NEW> l1_obj;//上帧输出的
            vector<OBJECT_INFO_NEW> l2_obj;//上上帧输出的
            if (isFirstFrame) {
                l1_obj.clear();
                l2_obj.clear();
            } else {
                switch (cacheAlgorithmMerge.size()) {
                    case 1: {
                        //只有1帧的话，只能取到上帧
                        l2_obj.clear();
                        l1_obj = cacheAlgorithmMerge.front();
                    }
                        break;
                    case 2: {
                        //2帧的话，可以取到上帧和上上帧，这里有上上帧出缓存的动作
                        l2_obj = cacheAlgorithmMerge.front();
                        cacheAlgorithmMerge.pop();
                        l1_obj = cacheAlgorithmMerge.front();
                    }
                        break;
                }
            }
            int num = merge_total(repateX, widthX, widthY, Xmax, Ymax, gatetx, gatety, gatex, gatey, isFirstFrame,
                                  mergeData.objInput.roadDataList.at(0).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(0).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(1).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(1).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(2).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(2).listObjs.size(),
                                  mergeData.objInput.roadDataList.at(3).listObjs.data(),
                                  mergeData.objInput.roadDataList.at(3).listObjs.size(),
                                  l1_obj.data(), l1_obj.size(), l2_obj.data(), l2_obj.size(),
                                  dataOut, angle_value);
            //将输出存入缓存
            vector<OBJECT_INFO_NEW> out;
            for (int i = 0; i < num; i++) {
                out.push_back(dataOut[i]);
            }
            cacheAlgorithmMerge.push(out);

            //将输出存入mergeData
            mergeData.objOutput.assign(out.begin(), out.end());
        }
            break;
    }

    return GetFusionData(mergeData);
}

int DataUnitFusionData::GetFusionData(MergeData mergeData) {
    const int INF = 0x7FFFFFFF;
    auto data = (Data *) this->owner;
    OType fusionData;
    fusionData.oprNum = random_uuid();
    fusionData.timestamp = mergeData.timestamp;
    fusionData.crossID = data->plateId;
    //算法输出量到FusionData.lstObjTarget
    for (auto iter: mergeData.objOutput) {
        ObjMix objMix;
        if (iter.objID1 != -INF) {
            OneRvWayObject rvWayObject;
            rvWayObject.wayNo = North;
            rvWayObject.roID = iter.objID1;
            rvWayObject.voID = iter.cameraID1;
            objMix.rvWayObject.push_back(rvWayObject);
        }
        if (iter.objID2 != -INF) {
            OneRvWayObject rvWayObject;
            rvWayObject.wayNo = East;
            rvWayObject.roID = iter.objID2;
            rvWayObject.voID = iter.cameraID2;
            objMix.rvWayObject.push_back(rvWayObject);
        }
        if (iter.objID3 != -INF) {
            OneRvWayObject rvWayObject;
            rvWayObject.wayNo = South;
            rvWayObject.roID = iter.objID3;
            rvWayObject.voID = iter.cameraID3;
            objMix.rvWayObject.push_back(rvWayObject);
        }
        if (iter.objID4 != -INF) {
            OneRvWayObject rvWayObject;
            rvWayObject.wayNo = West;
            rvWayObject.roID = iter.objID4;
            rvWayObject.voID = iter.cameraID4;
            objMix.rvWayObject.push_back(rvWayObject);
        }

        OBJECT_INFO_NEW2ObjMix(iter, objMix);

        fusionData.lstObjTarget.push_back(objMix);
    }

    bool isSendPicData = true;
    if (isSendPicData) {
        fusionData.isHasImage = 1;
        //算法输入量到FusionData.lstVideos
        for (auto iter: mergeData.objInput.roadDataList) {
            VideoData videoData;
            videoData.rvHardCode = iter.hardCode;
            videoData.imageData = iter.imageData;
            videoData.direction = iter.direction;
            for (auto iter1: iter.listObjs) {
                VideoTargets videoTargets;
                videoTargets.cameraObjID = iter1.cameraID;
                videoTargets.left = iter1.left;
                videoTargets.top = iter1.top;
                videoTargets.right = iter1.right;
                videoTargets.bottom = iter1.bottom;
                videoData.lstVideoTargets.push_back(videoTargets);
            }
            fusionData.lstVideos.push_back(videoData);
        }
    } else {
        fusionData.isHasImage = 0;
        fusionData.lstVideos.resize(0);
    }

    //做这次融合的总结
    Summary(&mergeData, &fusionData);

    if (!pushO(fusionData)) {
        VLOG(2) << this->name << " 队列已满，未存入数据 timestamp:" << (uint64_t) fusionData.timestamp;
    } else {
        VLOG(2) << this->name << " 数据存入 timestamp:" << (uint64_t) fusionData.timestamp;
    }

    return 0;
}

void DataUnitFusionData::Summary(MergeData *mergeData, FusionData *fusionData) {

    stringstream content;
    content << "\n";
    //1.打印表头
    content << "------------------Summary---------------\n";
    //2.打印标定时间戳
    content << "标定时间戳: " << (uint64_t) mergeData->timestamp << "\n";
    //3.打印输入数据
    content << "输入数据:\n";
    content << "\t" << std::setw(8) << "序号"
            << "\t" << std::setw(8) << "设备号"
            << "\t" << std::setw(8) << "方向"
            << "\t" << std::setw(8) << "时间戳"
            << "\t" << std::setw(8) << "目标数"
            << "\t" << std::setw(8) << "图片大小(base64)"
            << "\n";
    for (int i = 0; i < mergeData->objInput.roadDataList.size(); i++) {
        auto &iter = mergeData->objInput.roadDataList[i];
        content << "\t" << std::setw(8) << i
                << "\t" << std::setw(8) << iter.hardCode
                << "\t" << std::setw(8) << iter.direction
                << "\t" << std::setw(8) << iter.timestamp
                << "\t" << std::setw(8) << iter.listObjs.size()
                << "\t" << std::setw(8) << iter.imageData.size()
                << "\n";
    }
    //4.打印输出数据
    content << "输出数据:\n";
    content << "\t" << std::setw(8) << "设备号"
            << "\t" << std::setw(8) << "时间戳"
            << "\t" << std::setw(8) << "目标数"
            << "\n";
    content << "\t" << std::setw(8) << fusionData->crossID
            << "\t" << std::setw(8) << (uint64_t) fusionData->timestamp
            << "\t" << std::setw(8) << fusionData->lstObjTarget.size()
            << "\n";
    //5.打印耗时
    content << "耗时:\n";
    content << "\t" << std::setw(8) << "算法耗时"
            << "\t" << std::setw(8) << "总耗时"
            << "\n";
    content << "\t" << std::setw(8) << algorithmCost
            << "\t" << std::setw(8) << totalCost
            << "\n";
    LOG(INFO) << content.str();
}

void DataUnitFusionData::taskO() {
    //1.取数组织发送内容
    OType item;
    if (!this->popO(item)) {
        return;
    }
    //是否需要剔除图片
    bool isSendPIC = localConfig.isSendPIC;
    if (!isSendPIC) {
        item.isHasImage = 0;
        for (auto &iter: item.lstVideos) {
            iter.imageData.clear();
        }

    }

    auto data = (Data *) this->owner;
    uint32_t deviceNo = stoi(data->matrixNo.substr(0, 10));
    Pkg pkg;
    item.PkgWithoutCRC(this->sn, deviceNo, pkg);
    this->sn++;
    //2.发送
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == "client1") {
            if (cli.second->isNeedReconnect) {
                LOG(ERROR) << "未连接" << cli.second->server_ip << ":" << cli.second->server_port;
                return;
            }
            uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int ret = cli.second->SendBase(pkg);
            uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            switch (ret) {
                case 0: {
                    LOG(INFO) << this->name << " 发送成功 " << cli.second->server_ip << ":" << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -1: {
                    LOG(INFO) << this->name << " 发送失败,未获取锁 " << cli.second->server_ip << ":"
                              << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                case -2: {
                    LOG(INFO) << this->name << " 发送失败,send fail " << cli.second->server_ip << ":"
                              << cli.second->server_port
                              << ",发送开始时间:" << to_string(timestampStart)
                              << ",发送结束时间:" << to_string(timestampEnd)
                              << ",帧内时间:" << to_string((uint64_t) item.timestamp)
                              << ",耗时:" << (timestampEnd - timestampStart) << " ms";
                }
                    break;
                default: {

                }
                    break;
            }
        }
    }
}



