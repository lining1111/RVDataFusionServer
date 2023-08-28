//
// Created by lining on 2023/2/23.
//

#include "sqliteApi.h"
#include <glog/logging.h>
#include <string.h>

static int callback_db(void *ptr, int count) {
//    DBG("database is lock now,can not write/read.count= %d.",count);  //每次执行一次回调函数打印一次该信息
    usleep(100 * 1000);   //如果获取不到锁，等待100毫秒
    return 1;    //回调函数返回值为1，则将不断尝试操作数据库。
}

int closeDatabase(sqlite3 *db) {
    if (db == nullptr) {
        return -1;
    }
    sqlite3_close(db);

    return 0;
}

int execSql(sqlite3 *db, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param) {
    int result;
    char *errMsg;

    result = sqlite3_exec(db, sql_string, sql_callback, param, &errMsg);
    if (result != SQLITE_OK) {
        LOG(ERROR) << "exec sql:" << sql_string << "fail, err:" << errMsg;
        sqlite3_free(errMsg);
        return -1;
    }
    sqlite3_free(errMsg);

    return 0;
}

int execGetTable(sqlite3 *db, char *sql_string, char ***data, int *row, int *col) {
    int ret = 0;
    char *errMsg;
    ret = sqlite3_get_table(db, sql_string, data, row, col, &errMsg);
    if (ret != SQLITE_OK) {
        LOG(ERROR) << "exec get table sql:" << sql_string << "fail,err:" << errMsg;
        sqlite3_free(errMsg);
        return -1;
    }
    sqlite3_free(errMsg);

    return ret;
}

int dbFileExecSql(std::string dbFile, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param) {
    int ret = 0;
    sqlite3 *db;

    ret = sqlite3_open(dbFile.c_str(), &db);
    if (ret != SQLITE_OK) {
        LOG(ERROR) << "open database fail:" << dbFile;
        return -1;
    }

    sqlite3_busy_handler(db, callback_db, (void *) db);

    ret = execSql(db, sql_string, sql_callback, param);

    closeDatabase(db);

    return ret;
}

int dbFileExecSqlTable(std::string dbFile, char *sql_string, char ***data, int *row, int *col) {
    int ret = 0;
    sqlite3 *db;
    ret = sqlite3_open(dbFile.c_str(), &db);
    if (ret != SQLITE_OK) {
        LOG(ERROR) << "can not open db file:" << dbFile;
        return -1;
    }
    sqlite3_busy_handler(db, callback_db, db);
    ret = execGetTable(db, sql_string, data, row, col);
    closeDatabase(db);
    return ret;
}

void dbFreeTable(char **result) {
    if (result != nullptr) {
        sqlite3_free_table(result);
    }
}

int dbCheckORAddTable(std::string dbFile, std::string tableName) {
    char *estr = new char[1024];
    int ret = 0;

    snprintf(estr, 1024, "create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)", tableName.c_str());
    ret = dbFileExecSql(dbFile, estr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "sql exec fail:" << estr;
        delete[] estr;
        return -1;
    }
    delete[] estr;
    return 0;
}

int dbCheckOrAddColumn(std::string dbFile, std::string tableName,
                       int column_index, std::string columnName, std::string columnDescription) {
    char *estr = new char[1024];
    int ret = 0;
    int row = 0;
    int col = 0;
    char **sqldata;
    int index_real = -1;

    //获取表中所有column的名称
    memset(estr, 0, 1024);
    snprintf(estr, 1024, "pragma table_info('%s');", tableName.c_str());
    ret = dbFileExecSqlTable(dbFile, estr, &sqldata, &row, &col);
    if (ret == -1) {
        LOG(ERROR) << "sql exec fail:" << estr;
        delete[] estr;
        sqlite3_free_table(sqldata);
        return -1;
    }
    bool need_alter = true;
    if (ret == 0) {
        for (int temp = 0; temp < row; temp++) {
            int n_index = col * (temp + 1);
//            DBG("%s:%s table %s column[%d] name = %s", __FUNCTION__, db_path, table_name, temp, sqldata[n_index + 1]);
            if (columnName.compare(sqldata[n_index + 1]) == 0) {
                need_alter = false;
                index_real = temp;
                break;
            }
        }

        if (need_alter) {
            //添加一列
            sqlite3_free_table(sqldata);
            memset(estr, 0, 1024);
            snprintf(estr, 1024, "alter table %s add %s %s;",
                     tableName.c_str(),
                     columnName.c_str(),
                     columnDescription.c_str());
            LOG(INFO) << "sql exec:" << estr;
            ret = dbFileExecSqlTable(dbFile, estr, &sqldata, &row, &col);
            if (ret == -1) {
                LOG(ERROR) << "db sql exec fail" << estr;
                sqlite3_free_table(sqldata);
                return -1;
            }
        }
    }

    sqlite3_free_table(sqldata);
    if (need_alter) {
        return 0;
    }
    if (index_real == column_index) {
        return 0;
    }
    LOG(ERROR) << "column " << columnName << "index is" << column_index << "but index real is" << index_real << ",err";

    return -1;
}

int dbDeleteTable(std::string dbFile, const char *table_name) {
    LOG(INFO) << "delete table,db:" << dbFile << ",table:" << table_name;
    char *estr = new char[1024];
    int ret = 0;

    snprintf(estr, 1024, "drop table %s", table_name);
    ret = dbFileExecSql(dbFile, estr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "sql exec fail:" << estr;
        delete[] estr;
        return -1;
    }
    delete[] estr;
    return 0;
}

void db_timet2timestamp(time_t t, char *timestr) {
    time_t now = t;
    struct tm time_tm;

    localtime_r(&now, &time_tm);
    sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d",
            1900 + time_tm.tm_year,
            1 + time_tm.tm_mon,
            time_tm.tm_mday,
            time_tm.tm_hour,
            time_tm.tm_min,
            time_tm.tm_sec);
}

std::string db_timet2timestamp(time_t t) {
    time_t now = t;
    struct tm time_tm;
    char timestr[64] = {0};

    localtime_r(&now, &time_tm);
    memset(timestr, 0, 64);
    sprintf(timestr, "%04d-%02d-%02d %02d:%02d:%02d",
            1900 + time_tm.tm_year,
            1 + time_tm.tm_mon,
            time_tm.tm_mday,
            time_tm.tm_hour,
            time_tm.tm_min,
            time_tm.tm_sec);

    return std::string(timestr);
}
