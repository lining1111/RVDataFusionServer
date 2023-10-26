//
// Created by lining on 2023/10/26.
//


#include "dbInfo.h"
#include <glog/logging.h>
#include "os/os.h"
#include "sqliteApi.h"

int checkTable(std::string dbFile, const DBTableInfo *table, int column_size) {
    int ret = 0;
    LOG(INFO) << "db file:" << dbFile;
    os::CreatePath(dbFile);

    //检查数据库文件是否存在
    if (access(dbFile.c_str(), R_OK | W_OK | F_OK) != 0) {
        LOG(ERROR) << "db file not exist:" << dbFile;
        char *cmd = new char[512];
        memset(cmd, 0, 512);
        sprintf(cmd, "sqlite3 %s", dbFile.c_str());
        LOG(INFO) << "create db file,cmd=" << cmd;
        int result = os::runCmd(cmd);
        if (result < 0) {
            LOG(ERROR) << "exec cmd err" << cmd;
            delete[] cmd;
            return -1;
        }
        delete[] cmd;
    }

    if (column_size <= 0) {
        LOG(ERROR) << "column size err:" << column_size;
        return -1;
    }

    for (int i = 0; i < column_size; i++) {
        bool check_succeed = true;
        //先检查table
        ret = dbCheckORAddTable(dbFile, table[0].tableName);
        if (ret != 0) {
            LOG(ERROR) << "dbCheckORAddTable err";
            return -1;
        }
        for (int c_index = 0; c_index < column_size; c_index++) {
            ret = dbCheckOrAddColumn(dbFile, table[0].tableName,
                                     c_index + 1,
                                     table[c_index].columnName,
                                     table[c_index].columnDescription);
            if (ret != 0) {
                check_succeed = false;
                LOG(ERROR) << "dbCheckOrAddColumn err delete table and retry";
                dbDeleteTable(dbFile, table[0].tableName.c_str());
                break;
            }
        }
        if (check_succeed) {
            LOG(INFO) << " check table:" << table[0].tableName << "success,column size:" << column_size;
            return 0;
        }
    }
    LOG(ERROR) << " check table:" << table[0].tableName << "fail,column size:" << column_size;
    return -1;
}

int checkTableColumn(std::string dbPath, std::string tab_name, DBTableColInfo *tab_column, int check_num) {
    LOG(INFO) << "db file:" << dbPath;
    int rtn = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    int icol = 0;

    for (icol = 0; icol < check_num; icol++) {
        nrow = 2;
        ncol = 0;
        memset(sqlstr, 0x0, 1024);
        sprintf(sqlstr, "select * from sqlite_master where name='%s' and sql like '%%%s%%';",
                tab_name.c_str(), tab_column[icol].name.c_str());
        rtn = dbFileExecSqlTable(dbPath, sqlstr, &sqldata, &nrow, &ncol);
        if (rtn < 0) {
            LOG(ERROR) << "db sql:" << sqlstr << "fail";
            delete[] sqlstr;
            return -1;
        }
        if (nrow == 0) {
            //添加字段
            memset(sqlstr, 0x0, 1024);
            sprintf(sqlstr, "alter table %s add %s %s", tab_name.c_str(), tab_column[icol].name.c_str(),
                    tab_column[icol].type.c_str());
            rtn = dbFileExecSql(dbPath, sqlstr, nullptr, nullptr);
            if (rtn < 0) {
                LOG(ERROR) << "db fail,sql:" << sqlstr;
            } else {
                LOG(INFO) << "add column ok,sql:" << sqlstr;
            }
        }
        dbFreeTable(sqldata);
    }
    delete[] sqlstr;
    return 0;
}
