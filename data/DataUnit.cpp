//
// Created by lining on 11/21/22.
//

#include "DataUnit.h"
#include <fstream>
#include <cmath>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <future>
#include <glog/logging.h>
#include "Data.h"

DataUnitTrafficFlowGather::DataUnitTrafficFlowGather(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitTrafficFlowGather::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitTrafficFlowGather";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitTrafficFlowGather::task(void *local) {
    auto dataUnit = (DataUnitTrafficFlowGather *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), true);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void DataUnitTrafficFlowGather::FindOneFrame(DataUnitTrafficFlowGather *dataUnit, uint64_t toCacheCha, bool isFront) {
    //1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitTrafficFlowGather当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitTrafficFlowGather取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }

    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitTrafficFlowGather 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }
    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

int DataUnitTrafficFlowGather::ThreadGetDataInRange(DataUnitTrafficFlowGather *dataUnit,
                                                    int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                if (uint64_t(refer.timestamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timestamp;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame[index] = cur;
                            VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较旧但只有1帧，保留:"
                                    << (uint64_t) refer.timestamp;
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较旧，舍弃:"
                                << (uint64_t) refer.timestamp;
                    }
                } else if ((uint64_t(refer.timestamp) >= leftTimestamp) &&
                           (uint64_t(refer.timestamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timestamp;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame[index] = cur;
                        VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳在范围内，取出来:"
                                << (uint64_t) refer.timestamp;
                        isFind = true;
                    }
                } else if (uint64_t(refer.timestamp) > rightTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitTrafficFlowGather第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    return index;
}

int DataUnitTrafficFlowGather::TaskProcessOneFrame(DataUnitTrafficFlowGather *dataUnit) {
    auto data = (Data *) dataUnit->owner;
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = data->plateId;


    for (auto iter:dataUnit->oneFrame) {
        if (!iter.flowData.empty()) {
            for (auto iter1:iter.flowData) {
                item.trafficFlow.push_back(iter1);
            }
        }
    }
    int ret = 0;
    if (!dataUnit->pushO(item)) {
        VLOG(3) << "DataUnitTrafficFlowGather 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
        ret = -1;
    } else {
        VLOG(3) << "DataUnitTrafficFlowGather 数据存入 timestamp:" << (uint64_t) item.timestamp;
        ret = 0;
    }
    return ret;
}

DataUnitCrossTrafficJamAlarm::DataUnitCrossTrafficJamAlarm(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitCrossTrafficJamAlarm::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitCrossTrafficJamAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitCrossTrafficJamAlarm::task(void *local) {
    auto dataUnit = (DataUnitCrossTrafficJamAlarm *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), false);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void
DataUnitCrossTrafficJamAlarm::FindOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit, uint64_t toCacheCha, bool isFront) {
    //1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitCrossTrafficJamAlarm取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitCrossTrafficJamAlarm 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }

    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

//报警信息是从缓存中取到现在的时间段为止，如果数据为空，则空，如果有数据，判断是否有报警，有则设置为报警
int DataUnitCrossTrafficJamAlarm::ThreadGetDataInRange(DataUnitCrossTrafficJamAlarm *dataUnit,
                                                       int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;

    auto curTimestamp = (rightTimestamp - leftTimestamp) / 2 + leftTimestamp;
    bool isFind = false;
    IType cur;
    bool isFrameExist = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                auto data = (Data *) dataUnit->owner;
                if (data->plateId.empty()) {
                    data->plateId = refer.crossID;
                }


                cur.oprNum = random_uuid();
                cur.timestamp = curTimestamp;
                cur.crossID = data->plateId;
                cur.hardCode = refer.hardCode;
                cur.alarmType = 0;
                cur.alarmStatus = 0;
                if (uint64_t(refer.timestamp) < curTimestamp) {
                    isFrameExist = true;
                    if (refer.alarmType == 1) {
                        cur.alarmType = refer.alarmType;
                        cur.alarmStatus = refer.alarmStatus;
                        cur.alarmTime = refer.alarmTime;
                    }

                } else if (uint64_t(refer.timestamp) > curTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitCrossTrafficJamAlarm第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    if (isFrameExist) {
        //将当前路的所有信息缓存入对应的索引
        dataUnit->oneFrame[index] = cur;
    }

    return index;
}


int DataUnitCrossTrafficJamAlarm::TaskProcessOneFrame(DataUnitCrossTrafficJamAlarm *dataUnit) {
    auto data = (Data *) dataUnit->owner;
    OType item;
    item.oprNum = random_uuid();
    item.timestamp = dataUnit->curTimestamp;
    item.crossID = data->plateId;
    item.alarmType = 0;
    item.alarmStatus = 0;
    auto tp = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    item.alarmTime = ss.str();
    //判断是否有拥堵
    for (auto iter:dataUnit->oneFrame) {
        if (iter.alarmType == 1) {
            item.alarmType = iter.alarmType;
            item.alarmStatus = iter.alarmStatus;
            item.alarmTime = iter.alarmTime;
        }
    }

    int ret = 0;
    if (!dataUnit->pushO(item)) {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
        ret = -1;
    } else {
        VLOG(3) << "DataUnitCrossTrafficJamAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
        ret = 0;
    }
    return ret;
}

DataUnitFusionData::DataUnitFusionData(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitFusionData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitFusionData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitFusionData::task(void *local) {
    auto dataUnit = (DataUnitFusionData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程

        auto data = (Data *) dataUnit->owner;

        DataUnitFusionData::MergeType mergeType;
        if (data->isMerge) {
            mergeType = DataUnitFusionData::Merge;
        } else {
            mergeType = DataUnitFusionData::NotMerge;
        }
        FindOneFrame(dataUnit, (1000 * 60), mergeType, true);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}


void
DataUnitFusionData::FindOneFrame(DataUnitFusionData *dataUnit, uint64_t toCacheCha, MergeType mergeType, bool isFront) {
    //1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timstamp) < timestampThreshold) {
        VLOG(3) << "DataUnitFusionData当前时间戳:" <<
                (uint64_t) refer.timstamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timstamp;
    }
    VLOG(3) << "DataUnitFusionData取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //3取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }

    //打印下每一路取到的时间戳
    VLOG(3) << "====== fusionData oneFrame size:" << dataUnit->oneFrame.size();
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitFusionData 第" << i << "路取到的时间戳为" << (uint64_t) iter.timstamp;
        }
    }

    //调用后续的处理
    TaskProcessOneFrame(dataUnit, mergeType);
}

int DataUnitFusionData::ThreadGetDataInRange(DataUnitFusionData *dataUnit,
                                             int index, uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
//    Debug("第%d路 左值%lu 右值%lu", index, leftTimestamp, rightTimestamp);
    bool isFind = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitFusionData第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;
            if (dataUnit->frontI(refer, index)) {
                auto data = (Data*)dataUnit->owner;
                if (data->plateId.empty()){
                    data->plateId = refer.matrixNo;
                }

                if (uint64_t(refer.timstamp) < leftTimestamp) {
                    //在左值外
                    if (dataUnit->sizeI(index) == 1) {
                        //取用
                        IType cur;
                        if (dataUnit->popI(cur, index)) {
                            //记录当前路的时间戳
                            dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                            //将当前路的所有信息缓存入对应的索引
                            dataUnit->oneFrame[index] = cur;
                            VLOG(3) << "DataUnitFusionData第" << index << "路时间戳较旧但只有1帧，保留:"
                                    << (uint64_t) refer.timstamp;
                            isFind = true;
                        }
                    } else {
                        dataUnit->popI(refer, index);
                        VLOG(3) << "DataUnitFusionData第" << index << "路时间戳较旧，舍弃:"
                                << (uint64_t) refer.timstamp;
                    }
                } else if ((uint64_t(refer.timstamp) >= leftTimestamp) &&
                           (uint64_t(refer.timstamp) <= rightTimestamp)) {
                    //在范围内
                    IType cur;
                    if (dataUnit->popI(cur, index)) {
                        //记录当前路的时间戳
                        dataUnit->xRoadTimestamp[index] = (uint64_t) cur.timstamp;
                        //将当前路的所有信息缓存入对应的索引
                        dataUnit->oneFrame[index] = cur;
                        VLOG(3) << "DataUnitFusionData第" << index << "路时间戳在范围内，取出来:"
                                << (uint64_t) refer.timstamp;
                        isFind = true;
                    }
                } else if (uint64_t(refer.timstamp) > rightTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitFusionData第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timstamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

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
        for (auto iter1:iter.listObjs) {
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
    for (auto iter:mergeData.objInput.roadDataList) {
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
            for (auto iter1:iter.listObjs) {
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
                    for (auto iter1:iter.listObjs) {
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
    for (auto iter:mergeData.objOutput) {
        ObjMix objMix;
        objMix.objID = iter.showID;
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
        objMix.objType = iter.objType;
        objMix.objColor = 0;
        objMix.plates = string(iter.plate_number);
        objMix.plateColor = string(iter.plate_color);
        objMix.distance = atof(string(iter.distance).c_str());
        objMix.angle = iter.directionAngle;
        objMix.speed = iter.speed;
        objMix.locationX = iter.locationX;
        objMix.locationY = iter.locationY;
        objMix.longitude = iter.longitude;
        objMix.latitude = iter.latitude;
        objMix.flagNew = iter.flag_new;

        fusionData.lstObjTarget.push_back(objMix);
    }
    VLOG(3) << "算法输出量到FusionData---fusionData.lstObjTarget size :" << fusionData.lstObjTarget.size();
    bool isSendPicData = true;
    if (isSendPicData) {
        fusionData.isHasImage = 1;
        //算法输入量到FusionData.lstVideos
        for (auto iter:mergeData.objInput.roadDataList) {
            VideoData videoData;
            videoData.rvHardCode = iter.hardCode;
            videoData.imageData = iter.imageData;
            for (auto iter1:iter.listObjs) {
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


DataUnitIntersectionOverflowAlarm::DataUnitIntersectionOverflowAlarm(int c, int threshold_ms, int i_num, int cache,
                                                                     void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitIntersectionOverflowAlarm::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitIntersectionOverflowAlarm";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitIntersectionOverflowAlarm::task(void *local) {
    auto dataUnit = (DataUnitIntersectionOverflowAlarm *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        FindOneFrame(dataUnit, (1000 * 60), false);
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

void DataUnitIntersectionOverflowAlarm::FindOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit, uint64_t toCacheCha,
                                                     bool isFront) {
//1确定标定的时间戳
    IType refer;
    if (isFront) {
        dataUnit->frontI(refer, dataUnit->i_maxSizeIndex);
    } else {
        dataUnit->backI(refer, dataUnit->i_maxSizeIndex);
    }
    //1.1如果取到的时间戳不正常(时间戳，比现在的时间晚(缓存数乘以频率))
    auto now = std::chrono::system_clock::now();
    uint64_t timestampThreshold =
            (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() -
             (dataUnit->fs_i * dataUnit->cache) - toCacheCha);
    if (uint64_t(refer.timestamp) < timestampThreshold) {
        VLOG(3) << "DataUnitIntersectionOverflowAlarm当前时间戳:" <<
                (uint64_t) refer.timestamp << "小于缓存阈值:" << (uint64_t) timestampThreshold;
        dataUnit->curTimestamp = timestampThreshold;
    } else {
        dataUnit->curTimestamp = (uint64_t) refer.timestamp;
    }
    VLOG(3) << "DataUnitIntersectionOverflowAlarm取同一帧时,标定时间戳为:" << (uint64_t) dataUnit->curTimestamp;
    uint64_t leftTimestamp = dataUnit->curTimestamp - dataUnit->thresholdFrame;
    uint64_t rightTimestamp = dataUnit->curTimestamp + dataUnit->thresholdFrame;

    //2取数
    dataUnit->oneFrame.clear();
    dataUnit->oneFrame.resize(dataUnit->numI);
    vector<std::shared_future<int>> finishs;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        std::shared_future<int> finish = std::async(std::launch::async, ThreadGetDataInRange, dataUnit, i,
                                                    leftTimestamp,
                                                    rightTimestamp);
        finishs.push_back(finish);
    }

    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        finishs.at(i).wait();
    }
    //打印下每一路取到的时间戳
    for (int i = 0; i < dataUnit->oneFrame.size(); i++) {
        auto iter = dataUnit->oneFrame.at(i);
        if (!iter.oprNum.empty()) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 第" << i << "路取到的时间戳为" << (uint64_t) iter.timestamp;
        }
    }

    //调用后续的处理
    TaskProcessOneFrame(dataUnit);
}

int DataUnitIntersectionOverflowAlarm::ThreadGetDataInRange(DataUnitIntersectionOverflowAlarm *dataUnit, int index,
                                                            uint64_t leftTimestamp, uint64_t rightTimestamp) {
    //找到时间戳在范围内的，如果只有1帧数据切晚于标定值则取出，直到取空为止
    VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路 左值" << leftTimestamp << "右值" << rightTimestamp;

    auto curTimestamp = (rightTimestamp - leftTimestamp) / 2 + leftTimestamp;
    bool isFind = false;
    IType cur;
    bool isFrameExist = false;
    do {
        if (dataUnit->emptyI(index)) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路数据为空";
            isFind = true;
        } else {
            IType refer;

            if (dataUnit->frontI(refer, index)) {
                auto data = (Data *) dataUnit->owner;
                if (data->plateId.empty()){
                    data->plateId = refer.crossID;
                }

                cur.oprNum = random_uuid();
                cur.timestamp = curTimestamp;
                cur.crossID = data->plateId;
                cur.hardCode = refer.hardCode;
                cur.alarmType = 0;
                cur.alarmStatus = 0;
                if (uint64_t(refer.timestamp) < curTimestamp) {
                    isFrameExist = true;
                    if (refer.alarmType == 1) {
                        cur.alarmType = refer.alarmType;
                        cur.alarmStatus = refer.alarmStatus;
                        cur.alarmTime = refer.alarmTime;
                    }

                } else if (uint64_t(refer.timestamp) > curTimestamp) {
                    //在右值外
                    VLOG(3) << "DataUnitIntersectionOverflowAlarm第" << index << "路时间戳较新，保留:"
                            << (uint64_t) refer.timestamp;
                    isFind = true;
                }
            }
        }
    } while (!isFind);

    if (isFrameExist) {
        //将当前路的所有信息缓存入对应的索引
        dataUnit->oneFrame[index] = cur;
    }

    return index;
}

int DataUnitIntersectionOverflowAlarm::TaskProcessOneFrame(DataUnitIntersectionOverflowAlarm *dataUnit) {
    OType item;
    int ret = 0;
    //将各路数据都写入
    for (auto iter:dataUnit->oneFrame) {
        item = iter;
        if (!dataUnit->pushO(item)) {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
            ret = -1;
        } else {
            VLOG(3) << "DataUnitIntersectionOverflowAlarm 数据存入 timestamp:" << (uint64_t) item.timestamp;
        }
    }


    return ret;
}

DataUnitInWatchData_1_3_4::DataUnitInWatchData_1_3_4(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}


void DataUnitInWatchData_1_3_4::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_1_3_4";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitInWatchData_1_3_4::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_1_3_4 *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter:dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitInWatchData_1_3_4 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitInWatchData_1_3_4 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitInWatchData_2::DataUnitInWatchData_2(int c, int threshold_ms, int i_num, int cache, void *owner) :
        DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitInWatchData_2::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitInWatchData_2";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitInWatchData_2::task(void *local) {
    auto dataUnit = (DataUnitInWatchData_2 *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter:dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitInWatchData_2 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitInWatchData_2 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}

DataUnitStopLinePassData::DataUnitStopLinePassData(int c, int threshold_ms, int i_num, int cache, void *owner)
        : DataUnit(c, threshold_ms, i_num, cache, owner) {

}

void DataUnitStopLinePassData::init(int c, int threshold_ms, int i_num, int cache, void *owner) {
    DataUnit::init(c, threshold_ms, i_num, cache, owner);
    timerBusinessName = "DataUnitStopLinePassData";
    timerBusiness = new Timer(timerBusinessName);
    timerBusiness->start(fs_i, std::bind(task, this));
}

void DataUnitStopLinePassData::task(void *local) {
    auto dataUnit = (DataUnitStopLinePassData *) local;
    pthread_mutex_lock(&dataUnit->oneFrameMutex);
    int maxSize = 0;
    for (int i = 0; i < dataUnit->i_queue_vector.size(); i++) {
        if (dataUnit->i_queue_vector.at(i).size() > maxSize) {
            maxSize = dataUnit->i_queue_vector.at(i).size();
            dataUnit->i_maxSizeIndex = i;
        }
    }
    if (maxSize > dataUnit->cache) {
        //执行相应的流程
        for (auto &iter:dataUnit->i_queue_vector) {
            while (!iter.empty()) {
                IType cur;
                iter.pop(cur);
                OType item = cur;
                if (!dataUnit->pushO(item)) {
                    VLOG(3) << "DataUnitStopLinePassData 队列已满，未存入数据 timestamp:" << (uint64_t) item.timestamp;
                } else {
                    VLOG(3) << "DataUnitStopLinePassData 数据存入 timestamp:" << (uint64_t) item.timestamp;
                }
            }
        }
    }

    pthread_mutex_unlock(&dataUnit->oneFrameMutex);
}
