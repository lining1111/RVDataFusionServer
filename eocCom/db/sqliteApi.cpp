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


int openDatabase(const char *addr, sqlite3 **db) {
    int result;
    result = sqlite3_open(addr, db);
    if (result != SQLITE_OK) {
        LOG(ERROR) << "can not open db path:" << addr;
        return -1;
    }
    return 0;
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

int dbExecSql(const char *addr, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param) {
    int ret = 0;
    sqlite3 *db;

    ret = openDatabase(addr, &db);
    if (ret) {
        return -1;
    }

    sqlite3_busy_handler(db, callback_db, (void *) db);

    ret = execSql(db, sql_string, sql_callback, param);

    closeDatabase(db);

    return ret;
}

int dbFileExecSql(const char *sql_file, char *sql_string, SQLITE3_CALLBACK sql_callback, void *param) {
    int ret = 0;
    sqlite3 *db;

    ret = sqlite3_open(sql_file, &db);
    if (ret != SQLITE_OK) {
        LOG(ERROR) << "open database " << sql_file << "fail";
        return -1;
    }

    sqlite3_busy_handler(db, callback_db, (void *) db);

    ret = execSql(db, sql_string, sql_callback, param);

    closeDatabase(db);

    return ret;
}

int dbExecSqlTable(const char *addr, char *sql_string, char ***data, int *row, int *col) {
    int ret = 0;
    sqlite3 *db;

    ret = openDatabase(addr, &db);
    if (ret) {
        return -1;
    }

    sqlite3_busy_handler(db, callback_db, (void *) db);

    ret = execGetTable(db, sql_string, data, row, col);

    closeDatabase(db);

    return ret;
}

int dbFileExecSqlTable(const char *db_file, char *sql_string, char ***data, int *row, int *col) {
    int ret = 0;
    sqlite3 *db;
    ret = sqlite3_open(db_file, &db);
    if (ret != SQLITE_OK) {
        LOG(ERROR) << "can not open db file:" << db_file;
        return -1;
    }
    sqlite3_busy_handler(db, callback_db, db);
    ret = execGetTable(db, sql_string, data, row, col);
    closeDatabase(db);
    return 0;
}

void dbFreeTable(char **result) {
    if (result != nullptr) {
        sqlite3_free_table(result);
    }
}

int dbCheckORAddTable(const char *db_path, const char *table_name) {
    char *estr = new char[1024];
    int ret = 0;

    snprintf(estr, 1024, "create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)", table_name);
    ret = dbFileExecSql((char *) db_path, estr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "sql exec fail:" << estr;
        delete[] estr;
        return -1;
    }
    delete[] estr;
    return 0;
}

int dbCheckOrAddColumn(const char *db_path, const char *table_name, int column_index, const char *column_name,
                       const char *column_description) {
    char *estr = new char[1024];
    int ret = 0;
    int row = 0;
    int col = 0;
    char **sqdata;
    int index_real = -1;

    //获取表中所有column的名称
    memset(estr, 0, 1024);
    snprintf(estr, 1024, "pragma table_info('%s');", table_name);
    ret = dbFileExecSqlTable((char *) db_path, estr, &sqdata, &row, &col);
    if (ret == -1) {
        LOG(ERROR) << "sql exec fail:" << estr;
        delete[] estr;
        sqlite3_free_table(sqdata);
        return -1;
    }
    bool need_alter = true;
    if (ret == 0) {
        for (int temp = 0; temp < row; temp++) {
            int n_index = col * (temp + 1);
//            DBG("%s:%s table %s column[%d] name = %s", __FUNCTION__, db_path, table_name, temp, sqdata[n_index + 1]);
            if (strcmp(column_name, sqdata[n_index + 1]) == 0) {
                need_alter = false;
                index_real = temp;
                break;
            }
        }

        if (need_alter) {
            //添加一列
            sqlite3_free_table(sqdata);
            memset(estr, 0, 1024);
            snprintf(estr, 1024, "alter table %s add %s %s;", table_name, column_name, column_description);
            LOG(INFO) << "sql exec:" << estr;
            ret = dbFileExecSqlTable((char *) db_path, estr, &sqdata, &row, &col);
            if (ret == -1) {
                LOG(ERROR) << "db sql exec fail" << estr;
                sqlite3_free_table(sqdata);
                return -1;
            }
        }
    }

    sqlite3_free_table(sqdata);
    if (need_alter) {
        return 0;
    }
    if (index_real == column_index) {
        return 0;
    }
    LOG(ERROR) << "column " << column_name << "index is" << column_index << "but index real is" << index_real
               << ",err";

    return -1;
}

int dbDeleteTable(const char *db_path, const char *table_name) {
    LOG(INFO) << "delete table,db:" << db_path << ",table:" << table_name;
    char *estr = new char[1024];
    int ret = 0;

    snprintf(estr, 1024, "drop table %s", table_name);
    ret = dbFileExecSql((char *) db_path, estr, NULL, NULL);
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
