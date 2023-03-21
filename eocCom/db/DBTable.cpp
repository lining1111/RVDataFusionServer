//
// Created by lining on 2023/2/23.
//

#include <cstring>
#include "DBTable.h"
#include "sqliteApi.h"
#include <glog/logging.h>
#include <sys/stat.h>
#include <os/os.h>

DatabaseInfo CLParking = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/CLParking.db", "V_1_0_0"};
//DatabaseInfo RoadsideParking = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/RoadsideParking.db", "V1_0"};
DatabaseInfo eoc_configure = {PTHREAD_MUTEX_INITIALIZER, "./eoc_configure.db", "V_1_0_0"};

//核心板基础配置
static DBTableInfo base_set_table[] = {
        {"base_set", "DevIndex",                "INTERGE"},
        {"base_set", "City",                    "TEXT"},
        {"base_set", "IsUploadToPlatform",      "INTERGE"},
        {"base_set", "Is4Gmodel",               "INTERGE"},
        {"base_set", "IsIllegalCapture",        "INTERGE"},
        {"base_set", "IsPrintIntersectionName", "INTERGE"},
        {"base_set", "FilesServicePath",        "TEXT"},
        {"base_set", "FilesServicePort",        "INTERGE"},
        {"base_set", "MainDNS",                 "TEXT"},
        {"base_set", "AlternateDNS",            "TEXT"},
        {"base_set", "PlatformTcpPath",         "TEXT"},
        {"base_set", "PlatformTcpPort",         "INTERGE"},
        {"base_set", "PlatformHttpPath",        "TEXT"},
        {"base_set", "PlatformHttpPort",        "INTERGE"},
        {"base_set", "SignalMachinePath",       "TEXT"},
        {"base_set", "SignalMachinePort",       "INTERGE"},
        {"base_set", "IsUseSignalMachine",      "INTERGE"},
        {"base_set", "NtpServerPath",           "TEXT"},
        {"base_set", "FusionMainboardIp",       "TEXT"},
        {"base_set", "FusionMainboardPort",     "INTERGE"},
        {"base_set", "IllegalPlatformAddress",  "TEXT"},
        {"base_set", "Remarks",                 "TEXT"}};

//所属路口信息
static DBTableInfo belong_intersection_table[] = {
        {"belong_intersection", "Guid",        "TEXT"},
        {"belong_intersection", "Name",        "TEXT"},
        {"belong_intersection", "Type",        "INTERGE"},
        {"belong_intersection", "PlatId",      "TEXT"},
        {"belong_intersection", "XLength",     "REAL"},
        {"belong_intersection", "YLength",     "REAL"},
        {"belong_intersection", "LaneNumber",  "INTERGE"},
        {"belong_intersection", "Latitude",    "TEXT"},
        {"belong_intersection", "Longitude",   "TEXT"},

        {"belong_intersection", "FlagEast",    "INTERGE"},
        {"belong_intersection", "FlagSouth",   "INTERGE"},
        {"belong_intersection", "FlagWest",    "INTERGE"},
        {"belong_intersection", "FlagNorth",   "INTERGE"},
        {"belong_intersection", "DeltaXEast",  "REAL"},
        {"belong_intersection", "DeltaYEast",  "REAL"},
        {"belong_intersection", "DeltaXSouth", "REAL"},
        {"belong_intersection", "DeltaYSouth", "REAL"},
        {"belong_intersection", "DeltaXWest",  "REAL"},
        {"belong_intersection", "DeltaYWest",  "REAL"},
        {"belong_intersection", "DeltaXNorth", "REAL"},
        {"belong_intersection", "DeltaYNorth", "REAL"},
        {"belong_intersection", "WidthX",      "REAL"},
        {"belong_intersection", "WidthY",      "REAL"}};

//融合参数设置
static DBTableInfo fusion_para_set_table[] = {
        {"fusion_para_set", "IntersectionAreaPoint1X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint1Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint2X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint2Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint3X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint3Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint4X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint4Y", "REAL"}};

//关联设备
static DBTableInfo associated_equip_table[] = {
        {"associated_equip", "EquipType", "INTERGE"},
        {"associated_equip", "EquipCode", "TEXT"}};

int mkdirFromPath(std::string path) {
    char path_tmp[512] = {0};
    int len = path.size();
    if (len >= 512) {
        LOG(ERROR) << "path:" << path << "is too long to mkdir";
        return -1;
    }
    sprintf(path_tmp, "%s", path.c_str());

    for (int i = 1; i < len; i++) {
        if (path_tmp[i] == '/') {
            path_tmp[i] = 0;
            if (access(path_tmp, R_OK | W_OK | F_OK) != 0) {
                if (mkdir(path_tmp, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
                    LOG(ERROR) << "mkdir " << path_tmp << "fail";
                    return -1;
                }
            }
            path_tmp[i] = '/';
        }
    }

    return 0;
}


int tableInit(std::string path, std::string version) {
    int ret = 0;
    LOG(INFO) << "using database file path:" << path << ",version:" << version;
    LOG(INFO) << "config db file:" << path;
    bool isFindVersion = false;
    char *sqlStr = new char[1024 * 2];
    char **sqlData;
    int nRow = 0;//行
    int nCol = 0;//列
    std::string cur_version;

    //查看表结构
    memset(sqlStr, 0, 1024 * 2);
    snprintf(sqlStr, 1024 * 2 - 1, "select tbl_name from sqlite_master where type='table'");
    ret = dbFileExecSqlTable(path, sqlStr, &sqlData, &nRow, &nCol);
    if (ret < 0) {
        LOG(ERROR) << "db fail,sql:" << sqlStr;
        delete[] sqlStr;
        return -1;
    }

    //分析结果
    //寻找版本号
    for (int i = 0; i < nRow; i++) {
        std::string name = std::string(sqlData[i + 1]);
        if (name.rfind("V_", 0) == 0) {
            cur_version = std::string(sqlData[i + 1]);
            if (cur_version == version) {
                isFindVersion = true;
            }
            break;
        }
    }

    //没找到版本号
    if (!isFindVersion) {
        LOG(ERROR) << "check db version from" << path << "fail,get:" << cur_version << "local:" << version;

        //判断表是否存在创建表，并检查补充字段
        memset(sqlStr, 0, 1024 * 2);
        snprintf(sqlStr, 1024 * 2 - 1, "create table IF NOT EXISTS conf_version("
                                       "Id INTEGER PRIMARY KEY NOT NULL,"
                                       "version TEXT NOT NULL,"
                                       "time       TEXT NOT NULL)");
        ret = dbFileExecSql(path, sqlStr, nullptr, nullptr);
        if (ret < 0) {
            LOG(ERROR) << "db fail,sql:" << sqlStr;
            delete[] sqlStr;
            return -1;
        }
        //删除数据版本,登录成功后会主动要一次配置
        memset(sqlStr, 0, 1024 * 2);
        snprintf(sqlStr, 1024 * 2 - 1, "delete from conf_version");
        ret = dbFileExecSql(path, sqlStr, nullptr, nullptr);
        if (ret < 0) {
            LOG(ERROR) << "db sql:" << sqlStr << "fail";
            delete[] sqlStr;
            return -1;
        }

        //基础配置表
        ret = checkTable(eoc_configure.path, base_set_table, sizeof(base_set_table) / sizeof(DBTableInfo));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:base_set_table";
            delete[] sqlStr;
            return -1;
        }
        //所属路口信息表
        ret = checkTable(eoc_configure.path, belong_intersection_table,
                         sizeof(belong_intersection_table) / sizeof(DBTableInfo));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:belong_intersection_table";
            delete[] sqlStr;
            return -1;
        }
        //融合参数设置表
        ret = checkTable(eoc_configure.path, fusion_para_set_table,
                         sizeof(fusion_para_set_table) / sizeof(DBTableInfo));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:fusion_para_set_table";
            delete[] sqlStr;
            return -1;
        }
        //关联设备表
        ret = checkTable(eoc_configure.path, associated_equip_table,
                         sizeof(associated_equip_table) / sizeof(DBTableInfo));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:fusion_para_set_table";
            delete[] sqlStr;
            return -1;
        }

        //更新为新版本数据库版本表
        memset(sqlStr, 0x0, 1024);
        if (cur_version.empty()) {
            snprintf(sqlStr, 1024, "create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)",
                     eoc_configure.version.c_str());
        } else {
            snprintf(sqlStr, 1024, "alter table %s rename to %s",
                     cur_version.c_str(),
                     eoc_configure.version.c_str());
        }
        ret = dbFileExecSql(eoc_configure.path, sqlStr, nullptr, nullptr);
        if (ret < 0) {
            LOG(ERROR) << "db fail,sql:" << sqlStr;
            delete[] sqlStr;
            return -1;
        }
    } else {
        LOG(INFO) << "check db version:" << eoc_configure.version << " success, from " << eoc_configure.path;
    }

    delete[] sqlStr;

    return 0;
}

int checkTable(std::string dbFile, const DBTableInfo *table, int column_size) {
    int ret = 0;
    LOG(INFO) << "config db file:" << dbFile;
    ret = mkdirFromPath(dbFile);
    if (ret != 0) {
        LOG(ERROR) << "mkdir err from path:" << dbFile;
        return -1;
    }
    //检查数据库文件是否存在
    if (access(dbFile.c_str(), R_OK | W_OK | F_OK) != 0) {
        LOG(ERROR) << "db file not exsit:" << dbFile;
        char *cmd = new char[512];
        memset(cmd, 0, 512);
        sprintf(cmd, "sqlite3 %s", dbFile.c_str());
        LOG(INFO) << "create db file,cmd=" << cmd;
        int result = os::execute_command(cmd);
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

int checkTableColumn(std::string tab_name, DBTableColInfo *tab_column, int check_num) {
    LOG(INFO) << "config db file:" << eoc_configure.path;
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
        rtn = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
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
            rtn = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
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

int DBDataVersion::deleteFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char sqlstr[1024] = {0};
    memset(sqlstr, 0x0, 1024);
    sprintf(sqlstr, "delete from conf_version");

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db fail,sql:" << sqlstr;
        return -1;
    }
    return 0;
}

int DBDataVersion::insertToDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char sqlstr[1024] = {0};
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "insert into conf_version(version,time) values('%s','%s')",
             this->version.c_str(), this->time.c_str());

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db fail,sql:" << sqlstr;
        return -1;
    }
    return 0;
}

int DBDataVersion::selectFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char sqlstr[1024] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select version from conf_version where id=(select MIN(id) from conf_version)");
    ret = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db fail,sql:" << sqlstr;
        return -1;
    }
    if (nrow == 1) {
        this->version = sqldata[1] ? sqldata[1] : "";
    } else {
        this->version = "";
        LOG(ERROR) << "get version fail";
        dbFreeTable(sqldata);
        return 1;
    }
    dbFreeTable(sqldata);

    return 0;
}

int dbGetCloudInfo(std::string &server_path, int &server_port, std::string &file_server_path, int &file_server_port) {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int rtn = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select CloudServerPath,TransferServicePath,CloudServerPort,FileServicePort from TB_ParkingLot "
                    "where ID=(select MIN(ID) from base_set)");
    rtn = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0) {
        LOG(ERROR) << "db fail sql:" << sqlstr;
        delete[] sqlstr;
        return -1;
    }
    if (nrow == 1) {
        server_path = sqldata[ncol + 0] ? sqldata[ncol + 0] : "";
        file_server_path = sqldata[ncol + 1] ? sqldata[ncol + 1] : "";
        server_port = atoi(sqldata[ncol + 2] ? sqldata[ncol + 2] : "0");
        file_server_port = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
    } else {
        LOG(ERROR) << "db select count err,sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);

    return 0;
}

int dbGetUname(std::string &uname) {
    LOG(INFO) << "config db file:" << CLParking.path;
    int rtn = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)");
    rtn = dbFileExecSqlTable(CLParking.path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0) {
        LOG(ERROR) << "db fail,sql:" << sqlstr;
        delete[] sqlstr;
        return -1;
    }
    if (nrow == 1) {
        uname = sqldata[1] ? sqldata[1] : "";
    } else {
        LOG(ERROR) << "db select count err,sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);

    return 0;
}

int DBBaseSet::deleteFromDB() {
    LOG(INFO) << "config db file:" << this->db;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from base_set");

    ret = dbFileExecSql(this->db, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int DBBaseSet::insertToDB() {
    LOG(INFO) << "config db file:" << this->db;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "insert into base_set("
                           "DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,"
                           "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
                           "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
                           "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
                           "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress,Remarks) "
                           "values(%d,'%s',%d,%d,%d,%d,'%s',%d,'%s','%s',"
                           "'%s',%d,'%s',%d,'%s',%d,%d,'%s','%s',%d,'%s','%s')",
             this->Index,
             this->City.c_str(),
             this->IsUploadToPlatform,
             this->Is4Gmodel,
             this->IsIllegalCapture,
             this->IsPrintIntersectionName,
             this->FilesServicePath.c_str(),
             this->FilesServicePort,
             this->MainDNS.c_str(),
             this->AlternateDNS.c_str(),
             this->PlatformTcpPath.c_str(),
             this->PlatformTcpPort,
             this->PlatformHttpPath.c_str(),
             this->PlatformHttpPort,
             this->SignalMachinePath.c_str(),
             this->SignalMachinePort,
             this->IsUseSignalMachine,
             this->NtpServerPath.c_str(),
             this->FusionMainboardIp.c_str(),
             this->FusionMainboardPort,
             this->IllegalPlatformAddress.c_str(),
             this->Remarks.c_str());

    ret = dbFileExecSql(this->db, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int DBBaseSet::selectFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select id,DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,Remarks,"
                    "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
                    "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
                    "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
                    "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress "
                    "from base_set order by id desc limit 1");
    ret = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        this->Index = atoi(sqldata[ncol + 1] ? sqldata[ncol + 1] : "0");
        this->City = sqldata[ncol + 2] ? sqldata[ncol + 2] : "";
        this->IsUploadToPlatform = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
        this->Is4Gmodel = atoi(sqldata[ncol + 4] ? sqldata[ncol + 4] : "0");
        this->IsIllegalCapture = atoi(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0");
        this->Remarks = sqldata[ncol + 6] ? sqldata[ncol + 6] : "";
        this->IsPrintIntersectionName = atoi(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0");
        this->FilesServicePath = sqldata[ncol + 8] ? sqldata[ncol + 8] : "";
        this->FilesServicePort = atoi(sqldata[ncol + 9] ? sqldata[ncol + 9] : "0");
        this->MainDNS = sqldata[ncol + 10] ? sqldata[ncol + 10] : "";
        this->AlternateDNS = sqldata[ncol + 11] ? sqldata[ncol + 11] : "";
        this->PlatformTcpPath = sqldata[ncol + 12] ? sqldata[ncol + 12] : "";
        this->PlatformTcpPort = atoi(sqldata[ncol + 13] ? sqldata[ncol + 13] : "0");
        this->PlatformHttpPath = sqldata[ncol + 14] ? sqldata[ncol + 14] : "";
        this->PlatformHttpPort = atoi(sqldata[ncol + 15] ? sqldata[ncol + 15] : "0");
        this->SignalMachinePath = sqldata[ncol + 16] ? sqldata[ncol + 16] : "";
        this->SignalMachinePort = atoi(sqldata[ncol + 17] ? sqldata[ncol + 17] : "0");
        this->IsUseSignalMachine = atoi(sqldata[ncol + 18] ? sqldata[ncol + 18] : "0");
        this->NtpServerPath = sqldata[ncol + 19] ? sqldata[ncol + 19] : "";
        this->FusionMainboardIp = sqldata[ncol + 20] ? sqldata[ncol + 20] : "";
        this->FusionMainboardPort = atoi(sqldata[ncol + 21] ? sqldata[ncol + 21] : "0");
        this->IllegalPlatformAddress = sqldata[ncol + 22] ? sqldata[ncol + 22] : "";
    } else {
        LOG(ERROR) << "db select count err,sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int DBIntersection::deleteFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from belong_intersection");

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int DBIntersection::insertToDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024 * 2];

    memset(sqlstr, 0x0, 2048);
    snprintf(sqlstr, 2048, "insert into belong_intersection("
                           "Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
                           "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                           "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY) "
                           "values('%s','%s',%d,'%s',%f,%f,%d,'%s','%s',%d,%d,%d,%d,%f,%f,%f,%f,%f,"
                           "%f,%f,%f,%f,%f)",
             this->Guid.c_str(),
             this->Name.c_str(),
             this->Type,
             this->PlatId.c_str(),
             this->XLength,
             this->YLength,
             this->LaneNumber,
             this->Latitude.c_str(),
             this->Longitude.c_str(),
             this->FlagEast,
             this->FlagSouth,
             this->FlagWest,
             this->FlagNorth,
             this->DeltaXEast,
             this->DeltaYEast,
             this->DeltaXSouth,
             this->DeltaYSouth,
             this->DeltaXWest,
             this->DeltaYWest,
             this->DeltaXNorth,
             this->DeltaYNorth,
             this->WidthX,
             this->WidthY);

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int DBIntersection::selectFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024 * 4];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sprintf(sqlstr, "select id,Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
                    "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                    "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY "
                    "from belong_intersection order by id desc limit 1");
    ret = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        this->Guid = sqldata[ncol + 1] ? sqldata[ncol + 1] : "";
        this->Name = sqldata[ncol + 2] ? sqldata[ncol + 2] : "";
        this->Type = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
        this->PlatId = sqldata[ncol + 4] ? sqldata[ncol + 4] : "0";
        this->XLength = atof(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0.0");
        this->YLength = atof(sqldata[ncol + 6] ? sqldata[ncol + 6] : "0.0");
        this->LaneNumber = atoi(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0");
        this->Latitude = sqldata[ncol + 8] ? sqldata[ncol + 8] : "";
        this->Longitude = sqldata[ncol + 9] ? sqldata[ncol + 9] : "";
        this->FlagEast = atoi(sqldata[ncol + 10] ? sqldata[ncol + 10] : "0");
        this->FlagSouth = atoi(sqldata[ncol + 11] ? sqldata[ncol + 11] : "0");
        this->FlagWest = atoi(sqldata[ncol + 12] ? sqldata[ncol + 12] : "0");
        this->FlagNorth = atoi(sqldata[ncol + 13] ? sqldata[ncol + 13] : "0");
        this->DeltaXEast = atof(sqldata[ncol + 14] ? sqldata[ncol + 14] : "0.0");
        this->DeltaYEast = atof(sqldata[ncol + 15] ? sqldata[ncol + 15] : "0.0");
        this->DeltaXSouth = atof(sqldata[ncol + 16] ? sqldata[ncol + 16] : "0.0");
        this->DeltaYSouth = atof(sqldata[ncol + 17] ? sqldata[ncol + 17] : "0.0");
        this->DeltaXWest = atof(sqldata[ncol + 18] ? sqldata[ncol + 18] : "0.0");
        this->DeltaYWest = atof(sqldata[ncol + 19] ? sqldata[ncol + 19] : "0.0");
        this->DeltaXNorth = atof(sqldata[ncol + 20] ? sqldata[ncol + 20] : "0.0");
        this->DeltaYNorth = atof(sqldata[ncol + 21] ? sqldata[ncol + 21] : "0.0");
        this->WidthX = atof(sqldata[ncol + 22] ? sqldata[ncol + 22] : "0.0");
        this->WidthY = atof(sqldata[ncol + 23] ? sqldata[ncol + 23] : "0.0");
    } else {
        LOG(ERROR) << "db select count err,sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int DBFusionParaSet::deleteFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from fusion_para_set");

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int DBFusionParaSet::insertToDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "insert into fusion_para_set("
                           "IntersectionAreaPoint1X,IntersectionAreaPoint1Y,IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
                           "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,IntersectionAreaPoint4X,IntersectionAreaPoint4Y) "
                           "values(%f,%f,%f,%f,%f,%f,%f,%f)",
             this->IntersectionAreaPoint1X,
             this->IntersectionAreaPoint1Y,
             this->IntersectionAreaPoint2X,
             this->IntersectionAreaPoint2Y,
             this->IntersectionAreaPoint3X,
             this->IntersectionAreaPoint3Y,
             this->IntersectionAreaPoint4X,
             this->IntersectionAreaPoint4Y);

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int DBFusionParaSet::selectFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    memset(sqlstr, 0, 1024);
    sprintf(sqlstr,
            "select id,IntersectionAreaPoint1X,IntersectionAreaPoint1Y,IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
            "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,IntersectionAreaPoint4X,IntersectionAreaPoint4Y "
            "from fusion_para_set order by id desc limit 1");
    ret = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        this->IntersectionAreaPoint1X = atof(sqldata[ncol + 1] ? sqldata[ncol + 1] : "0.0");
        this->IntersectionAreaPoint1Y = atof(sqldata[ncol + 2] ? sqldata[ncol + 2] : "0.0");
        this->IntersectionAreaPoint2X = atof(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0.0");
        this->IntersectionAreaPoint2Y = atof(sqldata[ncol + 4] ? sqldata[ncol + 4] : "0.0");
        this->IntersectionAreaPoint3X = atof(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0.0");
        this->IntersectionAreaPoint3Y = atof(sqldata[ncol + 6] ? sqldata[ncol + 6] : "0.0");
        this->IntersectionAreaPoint4X = atof(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0.0");
        this->IntersectionAreaPoint4Y = atof(sqldata[ncol + 8] ? sqldata[ncol + 8] : "0.0");
    } else {
        LOG(ERROR) << "db select count err,sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int DBAssociatedEquip::deleteAllFromDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];
    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from associated_equip");

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int DBAssociatedEquip::insertToDB() {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024 - 1, "insert into associated_equip("
                               "EquipType,EquipCode) values (%d,'%s')",
             this->EquipType,
             this->EquipCode.c_str());

    ret = dbFileExecSql(eoc_configure.path, sqlstr, nullptr, nullptr);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int getAssociatedEquips(std::vector<DBAssociatedEquip> &data) {
    LOG(INFO) << "config db file:" << eoc_configure.path;
    int ret = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select id,EquipType,EquipCode from associated_equip");
    ret = dbFileExecSqlTable(eoc_configure.path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    data.clear();
    if (nrow <= 0) {
        LOG(ERROR) << "db select count err.sql:" << sqlstr << "fail";
    } else {
        DBAssociatedEquip tmp_data;
        for (int i = 0; i < nrow; i++) {
            int index = ncol * (i + 1);
            tmp_data.EquipType = atoi(sqldata[index + 1] ? sqldata[index + 1] : "0");
            tmp_data.EquipCode = sqldata[index + 2] ? sqldata[index + 2] : "";
            data.push_back(tmp_data);
        }
    }

    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}
