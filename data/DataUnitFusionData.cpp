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

using namespace std;

DataUnitFusionData::DataUnitFusionData(int c, int fs, int i_num, int cache, void *owner) :
        DataUnit(c, fs, i_num, cache, owner) {

}

void DataUnitFusionData::init(int c, int fs, int i_num, int cache, void *owner) {
    DataUnit::init(c, fs, i_num, cache, owner);
    LOG(INFO) << "DataUnitFusionData fs_i:" << this->fs_i;
    timerBusinessName = "DataUnitFusionData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(10, std::bind(task, this));
}

void DataUnitFusionData::task(void *local) {

    uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    auto dataUnit = (DataUnitFusionData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    int maxSizeIndex = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            maxSizeIndex = i;
        }
    }
    if (maxSize >= dataUnit->cache) {
        //执行相应的流程
        if (dataUnit->i_maxSizeIndex == -1) {
            //只有在第一次它为数据默认值-1的时候才执行赋值
            dataUnit->i_maxSizeIndex = maxSizeIndex;
        }

        auto data = (Data *) dataUnit->owner;

        DataUnitFusionData::MergeType mergeType;
        if (data->isMerge) {
            mergeType = DataUnitFusionData::Merge;
        } else {
            mergeType = DataUnitFusionData::NotMerge;
        }
        FindOneFrame(dataUnit, mergeType, dataUnit->cache / 2);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);

    uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    auto cost = timestampEnd - timestampStart;
//    LOG(INFO) << "DataUnitFusionData 取同一帧完成融合耗时" << cost << "ms";
}

void
DataUnitFusionData::FindOneFrame(DataUnitFusionData *dataUnit, MergeType mergeType, int offset) {
    //1确定标定的时间戳
    IType refer;
    if (!dataUnit->getIOffset(refer, dataUnit->i_maxSizeIndex, offset)) {
        dataUnit->i_maxSizeIndexNext();
        return;
    }
    //判断上次取的时间戳和这次的一样吗
    if ((dataUnit->timestampStore + dataUnit->fs_i) > ((uint64_t) refer.timstamp)) {
        dataUnit->taskSearchCount++;
        if ((dataUnit->taskSearchCount * dataUnit->timerBusiness->getPeriodMs()) >= (dataUnit->fs_i * 2.5)) {
            //超过阈值，切下一路,重新计数,还得是下一路的满缓存情况
            dataUnit->i_maxSizeIndexNext();
        }
        return;
    }
    LOG(INFO) << "DataUnitFusionData取同一帧时,标定路是:" << dataUnit->i_maxSizeIndex;
    dataUnit->taskSearchCount = 0;
    dataUnit->timestampStore = refer.timstamp;

    dataUnit->curTimestamp = refer.timstamp;

    std::time_t t((uint64_t) dataUnit->curTimestamp / 1000);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");

    LOG(INFO) << "DataUnitFusionData取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp << " " << ss.str();
    auto data = (Data *) dataUnit->owner;
    data->isStartFusion = true;
    //3取数
    vector<IType>().swap(dataUnit->oneFrame);
    dataUnit->oneFrame.resize(dataUnit->numI);
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        ThreadGetDataInRange(dataUnit, i, dataUnit->curTimestamp);
    }

    //将标定时间戳，和帧内时间戳输出到文件
    string line = "";
    line += to_string(dataUnit->curTimestamp);
    line += ",";

    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        line += to_string((uint64_t) iter.timstamp);
        line += ",";
        if (!iter.oprNum.empty()) {
            LOG(INFO) << "DataUnitFusionData 第" << i << "路取到的时间戳为" << (uint64_t) iter.timstamp;
        }
    }
    line += "\n";
    //写入文件
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

    //调用后续的处理
    TaskProcessOneFrame(dataUnit, mergeType);
}

int
DataUnitFusionData::ThreadGetDataInRange(DataUnitFusionData *dataUnit, int index, uint64_t curTimestamp) {
    //找到时间戳在范围内的帧
    if (dataUnit->emptyI(index)) {
        VLOG(3) << "DataUnitFusionData第" << index << "路数据为空";
    } else {
        for (int i = 0; i < dataUnit->sizeI(index); i++) {
            IType refer;
            if (dataUnit->getIOffset(refer, index, i)) {
                VLOG(3) << "DataUnitFusionData第" << index << "路时间戳:" << (uint64_t) refer.timstamp
                        << ",标定时间戳:" << curTimestamp;
                if (abs(((long long) refer.timstamp - (long long) curTimestamp)) < dataUnit->fs_i) {
                    //在范围内
                    //记录当前路的时间戳
                    dataUnit->xRoadTimestamp[index] = (uint64_t) refer.timstamp;
                    //将当前路的所有信息缓存入对应的索引
                    dataUnit->oneFrame[index] = refer;
                    VLOG(3) << "DataUnitFusionData第" << index << "路时间戳在范围内，取出来:"
                            << (uint64_t) refer.timstamp;
                    break;
                }
            }
        }
    }

    return index;
}

int DataUnitFusionData::TaskProcessOneFrame(DataUnitFusionData *dataUnit, DataUnitFusionData::MergeType mergeType) {
    //1，将同一帧内待输入量存入集合
    DataUnitFusionData::RoadDataInSet roadDataInSet;
    roadDataInSet.roadDataList.resize(dataUnit->numI);
//    printf("=====1 roadDataInSet.roadDataList size:%d\n", roadDataInSet.roadDataList.size());
    roadDataInSet.timestamp = dataUnit->curTimestamp;
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);

        DataUnitFusionData::RoadData item;
        item.hardCode = iter.hardCode;
        item.imageData = iter.imageData;
        item.direction = iter.direction;
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
//    printf("=====2 roadDataInSet.roadDataList size:%d\n", roadDataInSet.roadDataList.size());
    int ret = 0;
    switch (mergeType) {
        case DataUnitFusionData::NotMerge: {
            ret = dataUnit->TaskNotMerge(roadDataInSet);
        }
            break;
        case DataUnitFusionData::Merge: {
            ret = dataUnit->TaskMerge(roadDataInSet);
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

//    mergeData.objInput.roadDataList.resize(this->numI);
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
    int haveOneRoadDataCount = 0;
    for (auto iter: mergeData.objInput.roadDataList) {
        if (!iter.listObjs.empty()) {
            haveOneRoadDataCount++;
        }
    }
    VLOG(3) << "融合算法细节---有数据的路数:" << haveOneRoadDataCount;
    //存输入数据到文件
    if (0) {
        string fileName = to_string(mergeData.timestamp) + "_in.txt";
        ofstream inDataFile;
        string inDataFileName = "mergeData/" + fileName;
        inDataFile.open(inDataFileName);
        string content;
        for (int i = 0; i < mergeData.objInput.roadDataList.size(); i++) {
            auto iter = mergeData.objInput.roadDataList.at(i);

            string split = ",";
            for (auto iter1: iter.listObjs) {
                content.append(to_string(i) + split +
                               to_string(iter1.locationX) + split +
                               to_string(iter1.locationY) + split +
                               to_string(iter1.speedX) + split +
                               to_string(iter1.speedY) + split +
                               to_string(iter1.objID) + split +
                               to_string(iter1.objType));
                content.append("\n");
            }
            //存入图片
            string inDataPicFileName = "mergeData/" + to_string(mergeData.timestamp) + "_in_" + to_string(i) + ".jpeg";
            ofstream inDataPicFile;
            inDataPicFile.open(inDataPicFileName);
            if (inDataPicFile.is_open()) {
                inDataPicFile.write(iter.imageData.data(), iter.imageData.size());
                printf("融合数据图片写入\n");
                inDataPicFile.flush();
                inDataPicFile.close();
            }

        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) content.c_str(), content.size());
            printf("融合数据写入\n");
            inDataFile.flush();
            inDataFile.close();
        }
    }


    VLOG(3) << "融合算法细节---从原始数据拷贝到算法输入量的数组容量:" << mergeData.objInput.roadDataList.size();
    //这里是根据含有识别数据路数来操作输出量
    switch (haveOneRoadDataCount) {
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

//存输出数据到文件
    if (0) {
        string fileName = to_string((uint64_t) mergeData.timestamp) + "_out.txt";
        std::ofstream inDataFile;
        string inDataFileName = "mergeData/" + fileName;
        inDataFile.open(inDataFileName);
        string contentO;
        for (int j = 0; j < mergeData.objOutput.size(); j++) {
            string split = ",";
            auto iter = mergeData.objOutput.at(j);
            contentO.append(to_string(iter.objID1) + split +
                            to_string(iter.objID2) + split +
                            to_string(iter.objID3) + split +
                            to_string(iter.objID4) + split +
                            to_string(iter.showID) + split +
                            to_string(iter.cameraID1) + split +
                            to_string(iter.cameraID2) + split +
                            to_string(iter.cameraID3) + split +
                            to_string(iter.cameraID4) + split +
                            to_string(iter.locationX) + split +
                            to_string(iter.locationY) + split +
                            to_string(iter.speed) + split +
                            to_string(iter.flag_new));
            contentO.append("\n");
        }
        if (inDataFile.is_open()) {
            inDataFile.write((const char *) contentO.c_str(), contentO.size());
            inDataFile.flush();
            inDataFile.close();
        } else {
//          Error("打开文件失败:%s", inDataFileNameO.c_str());
        }
    }

    return GetFusionData(mergeData);
}

int DataUnitFusionData::GetFusionData(MergeData mergeData) {
    const int INF = 0x7FFFFFFF;
    auto data = (Data *) this->owner;
    OType fusionData;
    fusionData.oprNum = random_uuid();
    fusionData.timstamp = mergeData.timestamp;
    fusionData.crossID = data->plateId;
    VLOG(3) << "算法输出量到FusionData---算法输出量数组大小:" << mergeData.objOutput.size();
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
    VLOG(3) << "算法输出量到FusionData---fusionData.lstObjTarget size :" << fusionData.lstObjTarget.size();
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
    VLOG(3) << "算法输出量到FusionData---fusionData.lstVideos size:" << fusionData.lstVideos.size();

    if (pushO(fusionData)) {
        VLOG(3) << "DataUnitFusionData 队列已满，未存入数据 timestamp:" << (uint64_t) fusionData.timstamp;
    } else {
        VLOG(3) << "DataUnitFusionData 数据存入 timestamp:" << (uint64_t) fusionData.timstamp;
    }

    return 0;
}

