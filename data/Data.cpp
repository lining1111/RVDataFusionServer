//
// Created by lining on 2023/2/27.
//

#include <sqlite3.h>
#include "Data.h"

Data *Data::m_pInstance = nullptr;

Data *Data::instance() {

    if (m_pInstance == nullptr) {
        m_pInstance = new Data();

        std::unique_lock<std::mutex> lck(m_pInstance->mtx);
        //1读取本地属性
        m_pInstance->getMatrixNo();
        m_pInstance->getPlatId();

        //2初始化数据
        int cliNum = 4;
        m_pInstance->unOrder.resize(cliNum);

        m_pInstance->dataUnitHistoryFusionData.in.resize(cliNum);
        m_pInstance->dataUnitFusionData.init(30, 100, cliNum, 10, m_pInstance);//100ms一帧

        m_pInstance->dataUnitHistoryTrafficFlowGather.in.resize(cliNum);
        m_pInstance->dataUnitTrafficFlowGather.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧

        m_pInstance->dataUnitHistoryCrossTrafficJamAlarm.in.resize(cliNum);
        m_pInstance->dataUnitCrossTrafficJamAlarm.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧

        m_pInstance->dataUnitHistoryIntersectionOverflowAlarm.in.resize(cliNum);
        m_pInstance->dataUnitIntersectionOverflowAlarm.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧

        m_pInstance->dataUnitHistoryInWatchData_1_3_4.in.resize(cliNum);
        m_pInstance->dataUnitInWatchData_1_3_4.init(30, 1000, cliNum, 0, m_pInstance);

        m_pInstance->dataUnitHistoryInWatchData_2.in.resize(cliNum);
        m_pInstance->dataUnitInWatchData_2.init(30, 1000, cliNum, 0, m_pInstance);

        //开启数据时间戳历史监听线程
        m_pInstance->isRun = true;
        std::thread(startDataUnitHistoryPrint, m_pInstance, 10).detach();

        lck.unlock();
    }
    return m_pInstance;
}


int Data::FindIndexInUnOrder(const string in) {
    int index = -1;
    //首先遍历是否已经存在
    int alreadyExistIndex = -1;
    for (int i = 0; i < unOrder.size(); i++) {
        auto &iter = unOrder.at(i);
        if (iter == in) {
            alreadyExistIndex = i;
            break;
        }
    }
    if (alreadyExistIndex >= 0) {
        index = alreadyExistIndex;
    } else {
        //不存在就新加
        for (int i = 0; i < unOrder.size(); i++) {
            auto &iter = unOrder.at(i);
            if (iter.empty()) {
                iter = in;
                index = i;
                break;
            }
        }
    }

    return index;
}


int Data::getMatrixNo() {
    //打开数据库
    sqlite3 *db;
    string dbName;
#ifdef aarch64
    dbName = "/home/nvidianx/bin/CLParking.db";
#else
    dbName = "./db/CLParking.db";
#endif
    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite open fail db:" << dbName;
        sqlite3_close(db);
        return -1;
    }

    char *sql = "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)";
    char **result, *errmsg;
    int nrow, ncolumn;
    string columnName;
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite err:" << errmsg;
        sqlite3_free(errmsg);
        return -1;
    }
    for (int m = 0; m < nrow; m++) {
        for (int n = 0; n < ncolumn; n++) {
            columnName = string(result[n]);
            if (columnName == "UName") {
                matrixNo = STR(result[ncolumn + n + m * nrow]);
                break;
            }
        }
    }
    LOG(INFO) << "sn:" << matrixNo;
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}

int Data::getPlatId() {
    //打开数据库
    sqlite3 *db;
    string dbName;
    dbName = "./eoc_configure.db";
    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite open fail db:" << dbName;
        sqlite3_close(db);
        return -1;
    }

    char *sql = "select PlatId from belong_intersection order by id desc limit 1";
    char **result, *errmsg;
    int nrow, ncolumn;
    string columnName;
    rc = sqlite3_get_table(db, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite err:" << errmsg;
        sqlite3_free(errmsg);
        return -1;
    }
    for (int m = 0; m < nrow; m++) {
        for (int n = 0; n < ncolumn; n++) {
            columnName = string(result[n]);
            if (columnName == "PlatId") {
                plateId = STR(result[ncolumn + n + m * nrow]);
                break;
            }
        }
    }
    LOG(INFO) << "plateId:" << plateId;
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}

int Data::startDataUnitHistoryPrint(Data *local, int interval) {

    if (local == nullptr) {
        return -1;
    }
    LOG(INFO) << __FUNCTION__ << "run";
    while (local->isRun) {
        sleep(interval);

        //dataUnitHistoryFusionData
        {
            std::string prefix = "history FusionData";
            auto dataUnitHistory = &local->dataUnitHistoryFusionData;
            auto dataUnitLocal = &local->dataUnitFusionData;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    WatchData item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timstamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    WatchData item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timstamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                FusionData item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timstamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                FusionData item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timstamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

        //dataUnitHistoryTrafficFlowGather
        {
            std::string prefix = "history TrafficFlowGather";
            auto dataUnitHistory = &local->dataUnitHistoryTrafficFlowGather;
            auto dataUnitLocal = &local->dataUnitTrafficFlowGather;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    TrafficFlow item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timestamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    TrafficFlow item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timestamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                TrafficFlowGather item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timestamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                TrafficFlowGather item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timestamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

        //dataUnitHistoryCrossTrafficJamAlarm
        {
            std::string prefix = "history CrossTrafficJamAlarm";
            auto dataUnitHistory = &local->dataUnitHistoryCrossTrafficJamAlarm;
            auto dataUnitLocal = &local->dataUnitCrossTrafficJamAlarm;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    CrossTrafficJamAlarm item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timestamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    CrossTrafficJamAlarm item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timestamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                CrossTrafficJamAlarm item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timestamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                CrossTrafficJamAlarm item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timestamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

        //dataUnitHistoryIntersectionOverflowAlarm
        {
            std::string prefix = "history IntersectionOverflowAlarm";
            auto dataUnitHistory = &local->dataUnitHistoryIntersectionOverflowAlarm;
            auto dataUnitLocal = &local->dataUnitIntersectionOverflowAlarm;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    IntersectionOverflowAlarm item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timestamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    IntersectionOverflowAlarm item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timestamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                IntersectionOverflowAlarm item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timestamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                IntersectionOverflowAlarm item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timestamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

        //dataUnitHistoryInWatchData_1_3_4
        {
            std::string prefix = "history InWatchData_1_3_4";
            auto dataUnitHistory = &local->dataUnitHistoryInWatchData_1_3_4;
            auto dataUnitLocal = &local->dataUnitInWatchData_1_3_4;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    InWatchData_1_3_4 item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timestamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    InWatchData_1_3_4 item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timestamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                InWatchData_1_3_4 item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timestamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                InWatchData_1_3_4 item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timestamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

        //dataUnitHistoryInWatchData_2
        {
            std::string prefix = "history InWatchData_2";
            auto dataUnitHistory = &local->dataUnitHistoryInWatchData_2;
            auto dataUnitLocal = &local->dataUnitInWatchData_2;
            //in
            for (int i = 0; i < dataUnitHistory->in.size(); i++) {
                auto iter = dataUnitHistory->in.at(i);
                if (iter.last == 0) {
                    LOG(INFO) << prefix << " in start at index:" << i;

                    InWatchData_2 item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到last
                        iter.last = item.timestamp;
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                } else {
                    InWatchData_2 item;
                    if (dataUnitLocal->frontI(item, i)) {
                        //取头部时间戳到now，同时判断下last和now的情况
                        iter.now = item.timestamp;
                        if (iter.now <= iter.last) {
                            LOG(ERROR) << prefix << " in timestamp pass through at index:" << i;//时间先后顺序出现穿越现象
                        } else {
                            uint64_t dValue = iter.now - iter.last;
                            LOG(INFO) << prefix << " in timestamp at index:" << i << " dValue:" << dValue << " through:"
                                      << interval;
                            //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                            //允许丢1帧
                            if (dValue < (interval - dataUnitLocal->fs_i)) {
                                LOG(INFO) << prefix << "in 出现丢帧现象 at index:" << i;
                            }
                        }
                    } else {
                        LOG(INFO) << prefix << " in empty at index:" << i;
                    }
                }
            }
            //out
            auto iter = dataUnitHistory->out;
            if (iter.last == 0) {
                LOG(INFO) << prefix << " out start";

                InWatchData_2 item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到last
                    iter.last = item.timestamp;
                } else {
                    LOG(INFO) << prefix << " out empty";
                }
            } else {
                InWatchData_2 item;
                if (dataUnitLocal->frontO(item)) {
                    //取头部时间戳到now，同时判断下last和now的情况
                    iter.now = item.timestamp;
                    if (iter.now <= iter.last) {
                        LOG(ERROR) << prefix << " out timestamp pass through";//时间先后顺序出现穿越现象
                    } else {
                        uint64_t dValue = iter.now - iter.last;
                        LOG(INFO) << prefix << " out timestamp" << " dValue:" << dValue << " through:" << interval;
                        //可利用差值和（帧率×（interval/period））值做对比，看下是否有丢帧现象
                        //允许丢1帧
                        if (dValue < (interval - dataUnitLocal->fs_i)) {
                            LOG(INFO) << prefix << "out 出现丢帧现象";
                        }
                    }
                } else {
                    LOG(INFO) << prefix << " out empty at index:";
                }
            }
        }

    }
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}
