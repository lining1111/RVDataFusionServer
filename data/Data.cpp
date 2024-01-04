//
// Created by lining on 2023/2/27.
//

#include <sqlite3.h>
#include "Data.h"
#include "common/config.h"
#include <algorithm>

Data *Data::m_pInstance = nullptr;

Data *Data::instance() {

    if (m_pInstance == nullptr) {
        m_pInstance = new Data();

        std::unique_lock<std::mutex> lck(m_pInstance->mtx);
        //1读取本地属性
        m_pInstance->getMatrixNo();
        m_pInstance->getPlatId();

        //2初始化数据
        int cliNum = localConfig.roadNum;
        //周期性数据放入自适应帧率
        m_pInstance->dataUnitFusionData = new DataUnitFusionData();
        m_pInstance->dataUnitFusionData->setCapNumI(15, cliNum);
//        m_pInstance->dataUnitFusionData.init(10, 80, cliNum, 10, m_pInstance);//80ms一帧
        m_pInstance->dataUnitTrafficFlowGather = new DataUnitTrafficFlowGather();
        m_pInstance->dataUnitTrafficFlowGather->setCapNumI(3, cliNum);
//        m_pInstance->dataUnitTrafficFlowGather.init(30, 500, cliNum, 3, m_pInstance);//500ms一帧
        m_pInstance->dataUnitCrossTrafficJamAlarm = new DataUnitCrossTrafficJamAlarm();
        m_pInstance->dataUnitCrossTrafficJamAlarm->init(10, 500, cliNum, 1, m_pInstance,
                                                        "DataUnitCrossTrafficJamAlarm", 10 * 1000);//1000ms一帧

        m_pInstance->dataUnitIntersectionOverflowAlarm = new DataUnitIntersectionOverflowAlarm();
        m_pInstance->dataUnitIntersectionOverflowAlarm->init(10, 500, cliNum, 1, m_pInstance,
                                                             "DataUnitIntersectionOverflowAlarm", 10);//1000ms一帧

        m_pInstance->dataUnitInWatchData_1_3_4 = new DataUnitInWatchData_1_3_4();
        m_pInstance->dataUnitInWatchData_1_3_4->init(10, 500, cliNum, 1, m_pInstance,
                                                     "DataUnitInWatchData_1_3_4", 10);

        m_pInstance->dataUnitInWatchData_2 = new DataUnitInWatchData_2();
        m_pInstance->dataUnitInWatchData_2->init(10, 500, cliNum, 1, m_pInstance,
                                                 "DataUnitInWatchData_2", 10);

        m_pInstance->dataUnitStopLinePassData = new DataUnitStopLinePassData();
        m_pInstance->dataUnitStopLinePassData->init(10, 500, cliNum, 1, m_pInstance,
                                                    "DataUnitStopLinePassData", 10);

        m_pInstance->dataUnitAbnormalStopData = new DataUnitAbnormalStopData();
        m_pInstance->dataUnitAbnormalStopData->init(10, 500, cliNum, 1, m_pInstance,
                                                    "DataUnitAbnormalStopData", 10);

        m_pInstance->dataUnitLongDistanceOnSolidLineAlarm = new DataUnitLongDistanceOnSolidLineAlarm();
        m_pInstance->dataUnitLongDistanceOnSolidLineAlarm->init(10, 500, cliNum, 1, m_pInstance,
                                                                "DataUnitLongDistanceOnSolidLineAlarm", 10);

        m_pInstance->dataUnitHumanData = new DataUnitHumanData();
        m_pInstance->dataUnitHumanData->init(10, 500, cliNum, 1, m_pInstance,
                                             "DataUnitHumanData", 10);//这个cliNum待定

        m_pInstance->dataUnitHumanLitPoleData = new DataUnitHumanLitPoleData();
        m_pInstance->dataUnitHumanLitPoleData->setCapNumI(3, cliNum);
//        m_pInstance->dataUnitHumanLitPoleData->init(10, 500, cliNum, 1, m_pInstance,
//                                                    "DataUnitHumanLitPoleData", 10);//这个cliNum待定 1s1帧

        m_pInstance->dataUnitTrafficDetectorStatus = new DataUnitTrafficDetectorStatus();
        m_pInstance->dataUnitTrafficDetectorStatus->init(10, 500, cliNum, 1, m_pInstance,
                                                         "DataUnitTrafficDetectorStatus", 10);//这个cliNum待定

        m_pInstance->dataUnitCrossStageData.clear();

        m_pInstance->reSendQueue = new ReSendQueue();
        m_pInstance->isRun = true;
        lck.unlock();
    }
    return m_pInstance;
}

int Data::getMatrixNo() {
    //打开数据库
    sqlite3 *db;
    string dbName;
#ifdef aarch64
    dbName = "/home/nvidianx/bin/CLParking.db";
#else
    dbName = "CLParking.db";
#endif
    //open
    int rc = sqlite3_open(dbName.c_str(), &db);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "sqlite open fail db:" << dbName;
        sqlite3_close(db);
        return -1;
    }

    const char *sql = "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)";
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
    //处理下sn字符串中间可能带-的情况
    matrixNo.erase(std::remove(matrixNo.begin(), matrixNo.end(), '-'), matrixNo.end());

    LOG(WARNING) << "sn:" << matrixNo;
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

    const char *sql = "select PlatId from belong_intersection order by id desc limit 1";
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
    LOG(WARNING) << "plateId:" << plateId;
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}