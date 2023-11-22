//
// Created by lining on 2023/10/26.
//


#include "dbInfo.h"
#include <glog/logging.h>
#include "os/os.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/SQLite/SQLiteException.h"

using namespace Poco::Data;
using Poco::Data::Keywords::now;
using Poco::Data::Keywords::use;
using Poco::Data::Keywords::bind;
using Poco::Data::Keywords::into;
using Poco::Data::Keywords::limit;

#include <sqlite3.h>
int checkTable(std::string dbFile, const DBTableInfo *table, int column_size) {
    int ret = 0;
    LOG(INFO) << "db file:" << dbFile;
    os::CreatePath(dbFile);

    //检查数据库文件是否存在
    if (access(dbFile.c_str(), R_OK | W_OK | F_OK) != 0) {
        LOG(ERROR) << "db file not exist:" << dbFile;
        int ret1 = 0;
        sqlite3 *db;
        ret1 = sqlite3_open(dbFile.c_str(), &db);//这步如果db文件不存在，则会创建一个sqlite的db文件
        if (ret1 != SQLITE_OK) {
            LOG(ERROR) << "can not open db file:" << dbFile;
            return -1;
        }
        sqlite3_close(db);
    }

    if (column_size <= 0) {
        LOG(ERROR) << "column size err:" << column_size;
        return -1;
    }

    SQLite::Connector::registerConnector();
    try {
        Session session(SQLite::Connector::KEY, dbFile, 3);

        Statement stmt(session);

        //先检查table
        stmt << "create table IF NOT EXISTS " << table[0].tableName << "(id INTEGER PRIMARY KEY NOT NULL);";
        stmt.execute(true);

        for (int i = 0; i < column_size; i++) {
            //查看表内字段是否存在
            bool isColumnExist = false;
            stmt.reset(session);
            stmt << "pragma table_info(" << table[0].tableName << ");";
            stmt.execute(true);
            Poco::Data::RecordSet rs(stmt);
            for (auto iter:rs) {
                std::string name = iter.get(1).toString();
                if (name == table[i].columnName){
                    isColumnExist = true;
                    break;
                }
            }
            //如果不存在，增加表内字段
            if (!isColumnExist) {
                stmt << "alter table " << table[i].tableName <<
                     " add column " << table[i].columnName << " " << table[i].columnDescription << ";";
                stmt.execute(true);
            }
        }
    }
    catch (Poco::Data::ConnectionFailedException &ex) {
        LOG(ERROR) << ex.displayText();
        ret = -1;
    }
    catch (Poco::Data::SQLite::InvalidSQLStatementException &ex) {
        LOG(ERROR) << ex.displayText();
        ret = -1;
    }
    catch (Poco::Data::ExecutionException &ex) {
        LOG(ERROR) << ex.displayText();
        ret = -1;
    }
    catch (Poco::Exception &ex) {
        LOG(ERROR) << ex.displayText();
        ret = -1;
    }
    Poco::Data::SQLite::Connector::unregisterConnector();
    if (ret != 0) {
        LOG(ERROR) << " check table:" << table[0].tableName << " fail,column size:" << column_size;
    }
    return ret;
}