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

        m_pInstance->dataUnitFusionData.init(30, 80, cliNum, 10, m_pInstance);//80ms一帧
        m_pInstance->dataUnitTrafficFlowGather.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧
        m_pInstance->dataUnitCrossTrafficJamAlarm.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧
        m_pInstance->dataUnitIntersectionOverflowAlarm.init(30, 1000, cliNum, 10, m_pInstance);//1000ms一帧
        m_pInstance->dataUnitInWatchData_1_3_4.init(30, 1000, cliNum, 0, m_pInstance);
        m_pInstance->dataUnitInWatchData_2.init(30, 1000, cliNum, 0, m_pInstance);
        m_pInstance->dataUnitStopLinePassData.init(30, 1000, cliNum, 0, m_pInstance);
        m_pInstance->dataUnitCamera3516Alarm.init(30, 1000, cliNum, 0, m_pInstance);//这个cliNum待定

        //开启数据时间戳历史监听线程
        m_pInstance->isRun = true;
//        std::thread(startDataUnitHistoryPrint, m_pInstance, 10).detach();

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