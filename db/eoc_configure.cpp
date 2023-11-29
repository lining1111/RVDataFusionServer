//
// Created by lining on 2023/10/26.
//

#include "eoc_configure.h"
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

namespace eoc_configure {

    //整个eoc_configure的数据库，就是靠表V_XX来区别数据库的版本，靠表config_version中的字段来表示配置的下发时间和版本的
    DBInfo dbInfo = {
            .mtx = new std::mutex(),
            .path="./eoc_configure.db",
            .version="V_1_0_0"};

//核心板基础配置
    static DBTableInfo base_set_table[] = {
            {"base_set", "DevIndex",                "INTEGER"},
            {"base_set", "City",                    "TEXT"},
            {"base_set", "IsUploadToPlatform",      "INTEGER"},
            {"base_set", "Is4Gmodel",               "INTEGER"},
            {"base_set", "IsIllegalCapture",        "INTEGER"},
            {"base_set", "IsPrintIntersectionName", "INTEGER"},
            {"base_set", "FilesServicePath",        "TEXT"},
            {"base_set", "FilesServicePort",        "INTEGER"},
            {"base_set", "MainDNS",                 "TEXT"},
            {"base_set", "AlternateDNS",            "TEXT"},
            {"base_set", "PlatformTcpPath",         "TEXT"},
            {"base_set", "PlatformTcpPort",         "INTEGER"},
            {"base_set", "PlatformHttpPath",        "TEXT"},
            {"base_set", "PlatformHttpPort",        "INTEGER"},
            {"base_set", "SignalMachinePath",       "TEXT"},
            {"base_set", "SignalMachinePort",       "INTEGER"},
            {"base_set", "IsUseSignalMachine",      "INTEGER"},
            {"base_set", "NtpServerPath",           "TEXT"},
            {"base_set", "FusionMainboardIp",       "TEXT"},
            {"base_set", "FusionMainboardPort",     "INTEGER"},
            {"base_set", "IllegalPlatformAddress",  "TEXT"},
            {"base_set", "Remarks",                 "TEXT"}};

//所属路口信息
    static DBTableInfo belong_intersection_table[] = {
            {"belong_intersection", "Guid",        "TEXT"},
            {"belong_intersection", "Name",        "TEXT"},
            {"belong_intersection", "Type",        "INTEGER"},
            {"belong_intersection", "PlatId",      "TEXT"},
            {"belong_intersection", "XLength",     "REAL"},
            {"belong_intersection", "YLength",     "REAL"},
            {"belong_intersection", "LaneNumber",  "INTEGER"},
            {"belong_intersection", "Latitude",    "TEXT"},
            {"belong_intersection", "Longitude",   "TEXT"},

            {"belong_intersection", "FlagEast",    "INTEGER"},
            {"belong_intersection", "FlagSouth",   "INTEGER"},
            {"belong_intersection", "FlagWest",    "INTEGER"},
            {"belong_intersection", "FlagNorth",   "INTEGER"},
            {"belong_intersection", "DeltaXEast",  "REAL"},
            {"belong_intersection", "DeltaYEast",  "REAL"},
            {"belong_intersection", "DeltaXSouth", "REAL"},
            {"belong_intersection", "DeltaYSouth", "REAL"},
            {"belong_intersection", "DeltaXWest",  "REAL"},
            {"belong_intersection", "DeltaYWest",  "REAL"},
            {"belong_intersection", "DeltaXNorth", "REAL"},
            {"belong_intersection", "DeltaYNorth", "REAL"},
            {"belong_intersection", "WidthX",      "REAL"},
            {"belong_intersection", "WidthY",      "REAL"}
    };

//融合参数设置
    static DBTableInfo fusion_para_set_table[] = {
            {"fusion_para_set", "IntersectionAreaPoint1X", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint1Y", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint2X", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint2Y", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint3X", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint3Y", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint4X", "REAL"},
            {"fusion_para_set", "IntersectionAreaPoint4Y", "REAL"}
    };

//关联设备
    static DBTableInfo associated_equip_table[] = {
            {"associated_equip", "EquipType", "INTEGER"},
            {"associated_equip", "EquipCode", "TEXT"}
    };

#include <sqlite3.h>

    int tableInit() {
        std::string dbFile = dbInfo.path;
        os::CreatePath(os::getFolderPath(dbFile));

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
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        int ret = 0;
        LOG(INFO) << "using database file path:" << dbInfo.path << ",version:" << dbInfo.version;
        bool isFindVersion = false;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);

            Statement stmt(session);
            //寻找版本号
            std::string curVersion;
            stmt.reset(session);
            stmt << "select tbl_name from sqlite_master where type='table';";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                if (iter.get(0).toString().find("V_", 0) == 0) {
                    curVersion = iter.get(0).toString();
                    if (curVersion == dbInfo.version) {
                        isFindVersion = true;
                        break;
                    }
                }
            }
            //如果没找到版本号，则创建表
            if (!isFindVersion) {
                LOG(ERROR) << "check db version from" << dbInfo.path << " fail,get:" << curVersion << ",local:"
                           << dbInfo.version;
                //1.创建版本号表
                stmt.reset(session);
                stmt << "create table IF NOT EXISTS conf_version("
                        "Id INTEGER PRIMARY KEY NOT NULL,"
                        "version TEXT NOT NULL,"
                        "time       TEXT NOT NULL);";
                stmt.execute(true);
                //1.1删除版本号表的内容
                stmt << "delete from conf_version";
                stmt.execute(true);
                //2.基础配置表
                ret = checkTable(dbInfo.path, base_set_table, sizeof(base_set_table) / sizeof(DBTableInfo));
                if (ret != 0) {
                    LOG(ERROR) << "checkTable fail:base_set_table";
                }
                //3.所属路口信息表
                ret = checkTable(dbInfo.path, belong_intersection_table,
                                 sizeof(belong_intersection_table) / sizeof(DBTableInfo));
                if (ret != 0) {
                    LOG(ERROR) << "checkTable fail:belong_intersection_table";
                }
                //4.融合参数设置表
                ret = checkTable(dbInfo.path, fusion_para_set_table,
                                 sizeof(fusion_para_set_table) / sizeof(DBTableInfo));
                if (ret != 0) {
                    LOG(ERROR) << "checkTable fail:fusion_para_set_table";
                }
                //5.关联设备表
                ret = checkTable(dbInfo.path, associated_equip_table,
                                 sizeof(associated_equip_table) / sizeof(DBTableInfo));
                if (ret != 0) {
                    LOG(ERROR) << "checkTable fail:fusion_para_set_table";
                }
                //6.更新为新版本数据库版本表
                stmt.reset(session);
                if (curVersion.empty()) {
                    stmt << "create table IF NOT EXISTS " << dbInfo.version << "(id INTEGER PRIMARY KEY NOT NULL)";
                } else {
                    stmt << "alter table " << curVersion << " rename to " << dbInfo.version;
                }
                stmt.execute(true);
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

        return 0;
    }

    int DBDataVersion::deleteFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "delete from conf_version";
            stmt.execute(true);
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

        return 0;
    }

    int DBDataVersion::insertToDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "insert into conf_version(version,time) values(?,?)", use(this->version), use(this->time);
            stmt.execute(true);
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

        return 0;
    }

    int DBDataVersion::selectFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "select version from conf_version where id=(select MIN(id) from conf_version)";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                this->version = iter.get(0).toString();
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

        return 0;
    }

    int DBBaseSet::deleteFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "delete from base_set";
            stmt.execute(true);
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

        return 0;
    }

    int DBBaseSet::insertToDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "insert into base_set("
                    "DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,"
                    "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
                    "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
                    "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
                    "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress,Remarks) "
                    "values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                    use(this->Index), use(this->City), use(this->IsUploadToPlatform),
                    use(this->Is4Gmodel), use(this->IsIllegalCapture), use(this->IsPrintIntersectionName),
                    use(this->FilesServicePath), use(this->FilesServicePort), use(this->MainDNS),
                    use(this->AlternateDNS), use(this->PlatformTcpPath), use(this->PlatformTcpPort),
                    use(this->PlatformHttpPath), use(this->PlatformHttpPort), use(this->SignalMachinePath),
                    use(this->SignalMachinePort), use(this->IsUseSignalMachine), use(this->NtpServerPath),
                    use(this->FusionMainboardIp), use(this->FusionMainboardPort), use(this->IllegalPlatformAddress),
                    use(this->Remarks);
            stmt.execute(true);
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

        return 0;
    }

    int DBBaseSet::selectFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "select id,DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,Remarks,"
                    "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
                    "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
                    "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
                    "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress "
                    "from base_set order by id desc limit 1";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                this->Index = strtol(iter.get(1).toString().c_str(), nullptr, 10);
                this->City = iter.get(2).toString();
                this->IsUploadToPlatform = strtol(iter.get(3).toString().c_str(), nullptr, 10);
                this->Is4Gmodel = strtol(iter.get(4).toString().c_str(), nullptr, 10);
                this->IsIllegalCapture = strtol(iter.get(5).toString().c_str(), nullptr, 10);
                this->Remarks = iter.get(6).toString();
                this->IsPrintIntersectionName = strtol(iter.get(7).toString().c_str(), nullptr, 10);
                this->FilesServicePath = iter.get(8).toString();
                this->FilesServicePort = strtol(iter.get(9).toString().c_str(), nullptr, 10);
                this->MainDNS = iter.get(10).toString();
                this->AlternateDNS = iter.get(11).toString();
                this->PlatformTcpPath = iter.get(12).toString();
                this->PlatformTcpPort = strtol(iter.get(13).toString().c_str(), nullptr, 10);
                this->PlatformHttpPath = iter.get(14).toString();
                this->PlatformHttpPort = strtol(iter.get(15).toString().c_str(), nullptr, 10);
                this->SignalMachinePath = iter.get(16).toString();
                this->SignalMachinePort = strtol(iter.get(17).toString().c_str(), nullptr, 10);
                this->IsUseSignalMachine = strtol(iter.get(18).toString().c_str(), nullptr, 10);
                this->NtpServerPath = iter.get(19).toString();
                this->FusionMainboardIp = iter.get(20).toString();
                this->FusionMainboardPort = strtol(iter.get(21).toString().c_str(), nullptr, 10);
                this->IllegalPlatformAddress = iter.get(22).toString();
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

        return 0;
    }

    int DBIntersection::deleteFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "delete from belong_intersection";
            stmt.execute(true);
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

        return 0;
    }

    int DBIntersection::insertToDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "insert into belong_intersection(Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,"
                    "Latitude,Longitude,FlagEast,FlagSouth,FlagWest,FlagNorth,"
                    "DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                    "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY) "
                    "values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                    use(this->Guid), use(this->Name), use(this->Type),
                    use(this->PlatId), use(this->XLength), use(this->YLength),
                    use(this->LaneNumber), use(this->Latitude), use(this->Longitude),
                    use(this->FlagEast), use(this->FlagSouth), use(this->FlagWest),
                    use(this->FlagNorth), use(this->DeltaXEast), use(this->DeltaYEast),
                    use(this->DeltaXSouth), use(this->DeltaYSouth), use(this->DeltaXWest),
                    use(this->DeltaYWest), use(this->DeltaXNorth), use(this->DeltaYNorth),
                    use(this->WidthX), use(this->WidthY);
            stmt.execute(true);
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

        return 0;
    }

    int DBIntersection::selectFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "select id,Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
                    "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                    "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY "
                    "from belong_intersection order by id desc limit 1";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                this->Guid = iter.get(1).toString();
                this->Name = iter.get(2).toString();
                this->Type = strtol(iter.get(3).toString().c_str(), nullptr, 10);
                this->PlatId = iter.get(4).toString();
                this->XLength = strtof(iter.get(5).toString().c_str(), nullptr);
                this->YLength = strtof(iter.get(6).toString().c_str(), nullptr);
                this->LaneNumber = strtol(iter.get(7).toString().c_str(), nullptr, 10);
                this->Latitude = iter.get(8).toString();
                this->Longitude = iter.get(9).toString();
                this->FlagEast = strtol(iter.get(10).toString().c_str(), nullptr, 10);
                this->FlagSouth = strtol(iter.get(11).toString().c_str(), nullptr, 10);
                this->FlagWest = strtol(iter.get(12).toString().c_str(), nullptr, 10);
                this->FlagNorth = strtol(iter.get(13).toString().c_str(), nullptr, 10);
                this->DeltaXEast = strtof(iter.get(14).toString().c_str(), nullptr);
                this->DeltaYEast = strtof(iter.get(15).toString().c_str(), nullptr);
                this->DeltaXSouth = strtof(iter.get(16).toString().c_str(), nullptr);
                this->DeltaYSouth = strtof(iter.get(17).toString().c_str(), nullptr);
                this->DeltaXWest = strtof(iter.get(18).toString().c_str(), nullptr);
                this->DeltaYWest = strtof(iter.get(19).toString().c_str(), nullptr);
                this->DeltaXNorth = strtof(iter.get(20).toString().c_str(), nullptr);
                this->DeltaYNorth = strtof(iter.get(21).toString().c_str(), nullptr);
                this->WidthX = strtof(iter.get(22).toString().c_str(), nullptr);
                this->WidthY = strtof(iter.get(23).toString().c_str(), nullptr);
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

        return 0;
    }

    int DBFusionParaSet::deleteFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "delete from fusion_para_set";
            stmt.execute(true);
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

        return 0;
    }

    int DBFusionParaSet::insertToDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "insert into fusion_para_set(IntersectionAreaPoint1X,IntersectionAreaPoint1Y,"
                    "IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
                    "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,"
                    "IntersectionAreaPoint4X,IntersectionAreaPoint4Y) "
                    "values(?,?,?,?,?,?,?,?)",
                    use(this->IntersectionAreaPoint1X), use(this->IntersectionAreaPoint1Y),
                    use(this->IntersectionAreaPoint2X), use(this->IntersectionAreaPoint2Y),
                    use(this->IntersectionAreaPoint3X), use(this->IntersectionAreaPoint3Y),
                    use(this->IntersectionAreaPoint4X), use(this->IntersectionAreaPoint4Y);
            stmt.execute(true);
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

        return 0;
    }

    int DBFusionParaSet::selectFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "select id,IntersectionAreaPoint1X,IntersectionAreaPoint1Y,"
                    "IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
                    "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,"
                    "IntersectionAreaPoint4X,IntersectionAreaPoint4Y "
                    "from fusion_para_set order by id desc limit 1";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                this->IntersectionAreaPoint1X = strtof(iter.get(1).toString().c_str(), nullptr);
                this->IntersectionAreaPoint1Y = strtof(iter.get(2).toString().c_str(), nullptr);
                this->IntersectionAreaPoint2X = strtof(iter.get(3).toString().c_str(), nullptr);
                this->IntersectionAreaPoint2Y = strtof(iter.get(4).toString().c_str(), nullptr);
                this->IntersectionAreaPoint3X = strtof(iter.get(5).toString().c_str(), nullptr);
                this->IntersectionAreaPoint3Y = strtof(iter.get(6).toString().c_str(), nullptr);
                this->IntersectionAreaPoint4X = strtof(iter.get(7).toString().c_str(), nullptr);
                this->IntersectionAreaPoint4Y = strtof(iter.get(8).toString().c_str(), nullptr);
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

        return 0;
    }

    int DBAssociatedEquip::deleteAllFromDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "delete from associated_equip";
            stmt.execute(true);
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

        return 0;
    }

    int DBAssociatedEquip::insertToDB() {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "insert into associated_equip(EquipType,EquipCode) values (?,?)",
                    use(this->EquipType), use(this->EquipCode);
            stmt.execute(true);
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

        return 0;
    }

    int getAssociatedEquips(std::vector<DBAssociatedEquip> &data) {
        std::unique_lock<std::mutex> lock(*dbInfo.mtx);
        LOG(INFO) << "db file:" << dbInfo.path;
        int ret = 0;
        SQLite::Connector::registerConnector();
        try {
            Session session(SQLite::Connector::KEY, dbInfo.path, 3);
            Statement stmt(session);
            stmt.reset(session);
            stmt << "select id,EquipType,EquipCode from associated_equip";
            stmt.execute(true);
            RecordSet rs(stmt);
            for (auto iter: rs) {
                DBAssociatedEquip item;
                item.EquipType = strtol(iter.get(1).toString().c_str(), nullptr, 10);
                item.EquipCode = iter.get(2).toString();
                data.push_back(item);
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

        return 0;
    }

}