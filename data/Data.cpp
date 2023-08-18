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
        m_pInstance->dataUnitFusionData = new DataUnitFusionData(30, 80, cliNum, 15, m_pInstance);
//        m_pInstance->dataUnitFusionData.init(10, 80, cliNum, 10, m_pInstance);//80ms一帧
        m_pInstance->dataUnitTrafficFlowGather = new DataUnitTrafficFlowGather(30, 500, cliNum, 3, m_pInstance);
//        m_pInstance->dataUnitTrafficFlowGather.init(30, 500, cliNum, 3, m_pInstance);//500ms一帧
        m_pInstance->dataUnitCrossTrafficJamAlarm = new DataUnitCrossTrafficJamAlarm();
        m_pInstance->dataUnitCrossTrafficJamAlarm->init(10, 500, cliNum, 10, m_pInstance);//1000ms一帧

        m_pInstance->dataUnitIntersectionOverflowAlarm = new DataUnitIntersectionOverflowAlarm();
        m_pInstance->dataUnitIntersectionOverflowAlarm->init(10, 500, cliNum, 10, m_pInstance);//1000ms一帧

        m_pInstance->dataUnitInWatchData_1_3_4 = new DataUnitInWatchData_1_3_4();
        m_pInstance->dataUnitInWatchData_1_3_4->init(10, 500, cliNum, 1, m_pInstance);

        m_pInstance->dataUnitInWatchData_2 = new DataUnitInWatchData_2();
        m_pInstance->dataUnitInWatchData_2->init(10, 500, cliNum, 1, m_pInstance);

        m_pInstance->dataUnitStopLinePassData = new DataUnitStopLinePassData();
        m_pInstance->dataUnitStopLinePassData->init(10, 500, cliNum, 1, m_pInstance);

        m_pInstance->dataUnitAbnormalStopData = new DataUnitAbnormalStopData();
        m_pInstance->dataUnitAbnormalStopData->init(10, 500, cliNum, 1, m_pInstance);

        m_pInstance->dataUnitLongDistanceOnSolidLineAlarm = new DataUnitLongDistanceOnSolidLineAlarm();
        m_pInstance->dataUnitLongDistanceOnSolidLineAlarm->init(10, 500, cliNum, 1, m_pInstance);

        m_pInstance->dataUnitHumanData = new DataUnitHumanData();
        m_pInstance->dataUnitHumanData->init(10, 500, cliNum, 1, m_pInstance);//这个cliNum待定

        m_pInstance->dataUnitHumanLitPoleData = new DataUnitHumanLitPoleData();
        m_pInstance->dataUnitHumanLitPoleData->init(10, 500, cliNum, 1, m_pInstance);//这个cliNum待定

        //开启数据时间戳历史监听线程
        m_pInstance->isRun = true;
//        std::thread(startDataUnitHistoryPrint, m_pInstance, 10).detach();

        lck.unlock();
    }
    return m_pInstance;
}

int Data::getMatrixNo() {
    //打开数据库
    sqlite3 *db;
    string dbName;
    dbName = "/home/nvidianx/bin/CLParking.db";
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
    LOG(INFO) << "plateId:" << plateId;
    sqlite3_free_table(result);
    sqlite3_close(db);
    return 0;
}