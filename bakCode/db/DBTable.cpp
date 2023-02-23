//
// Created by lining on 2023/2/23.
//

#include <cstring>
#include "DBTable.h"
#include "sqliteApi.h"
#include <glog/logging.h>
#include <sys/stat.h>

Database roadeside_parking_db = {PTHREAD_MUTEX_INITIALIZER, "./eoc_configure.db", "V_1_0_0"};
Database cl_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/CLParking.db", "V_1_0_0"};
Database factory_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/RoadsideParking.db", "V_1_0_0"};

//核心板基础配置
static DBT_Table base_set_table[] = {
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
static DBT_Table belong_intersection_table[] = {
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
static DBT_Table fusion_para_set_table[] = {
        {"fusion_para_set", "IntersectionAreaPoint1X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint1Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint2X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint2Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint3X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint3Y", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint4X", "REAL"},
        {"fusion_para_set", "IntersectionAreaPoint4Y", "REAL"}};

//关联设备
static DBT_Table associated_equip_table[] = {
        {"associated_equip", "EquipType", "INTERGE"},
        {"associated_equip", "EquipCode", "TEXT"}};

int mkdirFromPath(const char *path) {
    char path_tmp[512] = {0};
    int len = strlen(path);
    if (len >= 512) {
        LOG(ERROR) << "path:" << path << "is too long to mkdir";
        return -1;
    }
    sprintf(path_tmp, "%s", path);

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
    int count = 1;
    bool isFindVersion = false;
    char *sqlStr = new char[1024 * 2];
    char **sqlData;
    int nRow = 0;//行
    int nCol = 0;//列
    std::string cur_version;

    //查看表结构
    memset(sqlStr, 0, 1024 * 2);
    snprintf(sqlStr, 1024 * 2 - 1, "select tbl_name from sqlite_master where type='table'");
    ret = dbFileExecSqlTable(path.c_str(), sqlStr, &sqlData, &nRow, &nCol);
    if (ret < 0) {
        LOG(ERROR) << "db sql table" << sqlStr << "fail";
        delete[] sqlStr;
        return -1;
    }

    //分析结果
    //寻找版本号
    for (int i = 0; i < nRow; i++) {
        if (strstr(sqlData[i + 1], "V_")) {
            cur_version = std::string(sqlData[i + 1]);
            if (cur_version == version) {
                count = 0;
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
        ret = dbFileExecSql(path.c_str(), sqlStr, nullptr, nullptr);
        if (ret < 0) {
            LOG(ERROR) << "db sql:" << sqlStr << "fail";
            delete[] sqlStr;
            return -1;
        }
        //删除数据版本,登录成功后会主动要一次配置
        memset(sqlStr, 0, 1024 * 2);
        snprintf(sqlStr, 1024 * 2 - 1, "delete from conf_version");
        ret = dbFileExecSql(path.c_str(), sqlStr, nullptr, nullptr);
        if (ret < 0) {
            LOG(ERROR) << "db sql:" << sqlStr << "fail";
            delete[] sqlStr;
            return -1;
        }

        //基础配置表
        ret = checkTable(roadeside_parking_db.path.c_str(), base_set_table, sizeof(base_set_table) / sizeof(DBT_Table));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:base_set_table";
            delete[] sqlStr;
            return -1;
        }
        //所属路口信息表
        ret = checkTable(roadeside_parking_db.path.c_str(), belong_intersection_table,
                         sizeof(belong_intersection_table) / sizeof(DBT_Table));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:belong_intersection_table";
            delete[] sqlStr;
            return -1;
        }
        //融合参数设置表
        ret = checkTable(roadeside_parking_db.path.c_str(), fusion_para_set_table,
                         sizeof(fusion_para_set_table) / sizeof(DBT_Table));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:fusion_para_set_table";
            delete[] sqlStr;
            return -1;
        }
        //关联设备表
        ret = checkTable(roadeside_parking_db.path.c_str(), associated_equip_table,
                         sizeof(associated_equip_table) / sizeof(DBT_Table));
        if (ret != 0) {
            LOG(ERROR) << "checkTable fail:fusion_para_set_table";
            delete[] sqlStr;
            return -1;
        }

        //更新为新版本数据库版本表
        memset(sqlStr, 0x0, 1024);
        if (cur_version.empty()) {
            snprintf(sqlStr, 1024, "create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)",
                     roadeside_parking_db.version.c_str());
        } else {
            snprintf(sqlStr, 1024, "alter table %s rename to %s", cur_version.c_str(),
                     roadeside_parking_db.version.c_str());
        }
        ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlStr, NULL, NULL);
        if (ret < 0) {
            LOG(ERROR) << "db sql err:" << sqlStr;
            delete[] sqlStr;
            return -1;
        }
    } else {
        LOG(INFO) << "check db version:" << roadeside_parking_db.version << " success, from "
                  << roadeside_parking_db.path;
    }

    delete[] sqlStr;

    return 0;
}

int checkTable(const char *db_path, const DBT_Table *table, int column_size) {
    int ret = 0;

    ret = mkdirFromPath(db_path);
    if (ret != 0) {
        LOG(ERROR) << "mkdir err from path:" << db_path;
        return -1;
    }

    if (column_size <= 0) {
        LOG(ERROR) << "column size err:" << column_size;
        return -1;
    }

    for (int i = 0; i < column_size; i++) {
        bool check_succeed = true;
        ret = dbCheckORAddTable(db_path, table[0].tableName.c_str());
        if (ret != 0) {
            LOG(ERROR) << "dbCheckORAddTable err";
            return -1;
        }
        for (int c_index = 0; c_index < column_size; c_index++) {
            ret = dbCheckOrAddColumn(db_path, table[0].tableName.c_str(), c_index + 1,
                                     table[c_index].columnName.c_str(),
                                     table[c_index].columnDescription.c_str());
            if (ret != 0) {
                check_succeed = false;
                LOG(ERROR) << "dbCheckOrAddColumn err delete table and retry";
                dbDeleteTable(db_path, table[0].tableName.c_str());
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

int checkTableColumn(std::string tab_name, DB_Table_Configure *tab_column, int check_num) {
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
        rtn = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
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
            rtn = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
            if (rtn < 0) {
                LOG(ERROR) << "db sql:" << sqlstr << "fail";
            } else {
                LOG(INFO) << "add column ok,sql:" << sqlstr;
            }
        }
        dbFreeTable(sqldata);
    }
    delete[] sqlstr;
    return 0;
}


int deleteVersion() {
    int ret = 0;
    char sqlstr[1024] = {0};
    memset(sqlstr, 0x0, 1024);
    sprintf(sqlstr, "delete from conf_version");

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        return -1;
    }
    return 0;
}

int insertVersion(DB_Conf_Data_Version &data) {
    int ret = 0;
    char sqlstr[1024] = {0};
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "insert into conf_version(version,time) values('%s','%s')",
             data.version.c_str(), data.time.c_str());

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        return -1;
    }
    return 0;
}

int getVersion(std::string &version) {
    int ret = 0;
    char sqlstr[1024] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select version from conf_version where id=(select MIN(id) from conf_version)");
    ret = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        return -1;
    }
    if (nrow == 1) {
        version = sqldata[1] ? sqldata[1] : "";
    } else {
        version = "";
        LOG(ERROR) << "get version fail";
        dbFreeTable(sqldata);
        return 1;
    }
    dbFreeTable(sqldata);

    return 0;
}

int delete_base_set() {
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from base_set");

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int insert_base_set(DB_Base_Set_T data) {
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
             data.Index, data.City.c_str(), data.IsUploadToPlatform, data.Is4Gmodel,
             data.IsIllegalCapture, data.IsPrintIntersectionName,
             data.FilesServicePath.c_str(), data.FilesServicePort, data.MainDNS.c_str(),
             data.AlternateDNS.c_str(), data.PlatformTcpPath.c_str(), data.PlatformTcpPort,
             data.PlatformHttpPath.c_str(), data.PlatformHttpPort, data.SignalMachinePath.c_str(),
             data.SignalMachinePort, data.IsUseSignalMachine, data.NtpServerPath.c_str(),
             data.FusionMainboardIp.c_str(), data.FusionMainboardPort, data.IllegalPlatformAddress.c_str(),
             data.Remarks.c_str());

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int get_base_set(DB_Base_Set_T &data) {
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
    ret = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        data.Index = atoi(sqldata[ncol + 1] ? sqldata[ncol + 1] : "0");
        data.City = sqldata[ncol + 2] ? sqldata[ncol + 2] : "";
        data.IsUploadToPlatform = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
        data.Is4Gmodel = atoi(sqldata[ncol + 4] ? sqldata[ncol + 4] : "0");
        data.IsIllegalCapture = atoi(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0");
        data.Remarks = sqldata[ncol + 6] ? sqldata[ncol + 6] : "";
        data.IsPrintIntersectionName = atoi(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0");
        data.FilesServicePath = sqldata[ncol + 8] ? sqldata[ncol + 8] : "";
        data.FilesServicePort = atoi(sqldata[ncol + 9] ? sqldata[ncol + 9] : "0");
        data.MainDNS = sqldata[ncol + 10] ? sqldata[ncol + 10] : "";
        data.AlternateDNS = sqldata[ncol + 11] ? sqldata[ncol + 11] : "";
        data.PlatformTcpPath = sqldata[ncol + 12] ? sqldata[ncol + 12] : "";
        data.PlatformTcpPort = atoi(sqldata[ncol + 13] ? sqldata[ncol + 13] : "0");
        data.PlatformHttpPath = sqldata[ncol + 14] ? sqldata[ncol + 14] : "";
        data.PlatformHttpPort = atoi(sqldata[ncol + 15] ? sqldata[ncol + 15] : "0");
        data.SignalMachinePath = sqldata[ncol + 16] ? sqldata[ncol + 16] : "";
        data.SignalMachinePort = atoi(sqldata[ncol + 17] ? sqldata[ncol + 17] : "0");
        data.IsUseSignalMachine = atoi(sqldata[ncol + 18] ? sqldata[ncol + 18] : "0");
        data.NtpServerPath = sqldata[ncol + 19] ? sqldata[ncol + 19] : "";
        data.FusionMainboardIp = sqldata[ncol + 20] ? sqldata[ncol + 20] : "";
        data.FusionMainboardPort = atoi(sqldata[ncol + 21] ? sqldata[ncol + 21] : "0");
        data.IllegalPlatformAddress = sqldata[ncol + 22] ? sqldata[ncol + 22] : "";
    } else {
        LOG(ERROR) << "db sql:" << sqlstr << "fail,select count err";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int delete_belong_intersection() {
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from belong_intersection");

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int insert_belong_intersection(DB_Intersection_t data) {
    int ret = 0;
    char *sqlstr = new char[1024 * 2];

    memset(sqlstr, 0x0, 2048);
    snprintf(sqlstr, 2048, "insert into belong_intersection("
                           "Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
                           "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                           "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY) "
                           "values('%s','%s',%d,'%s',%f,%f,%d,'%s','%s',%d,%d,%d,%d,%f,%f,%f,%f,%f,"
                           "%f,%f,%f,%f,%f)",
             data.Guid.c_str(), data.Name.c_str(), data.Type, data.PlatId.c_str(),
             data.XLength, data.YLength, data.LaneNumber,
             data.Latitude.c_str(), data.Longitude.c_str(), data.FlagEast, data.FlagSouth, data.FlagWest,
             data.FlagNorth,
             data.DeltaXEast, data.DeltaYEast, data.DeltaXSouth, data.DeltaYSouth,
             data.DeltaXWest, data.DeltaYWest, data.DeltaXNorth, data.DeltaYNorth, data.WidthX, data.WidthY);

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int get_belong_intersection(DB_Intersection_t &data) {
    int ret = 0;
    char *sqlstr = new char[1024 * 4];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sprintf(sqlstr, "select id,Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
                    "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
                    "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY "
                    "from belong_intersection order by id desc limit 1");
    ret = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        data.Guid = sqldata[ncol + 1] ? sqldata[ncol + 1] : "";
        data.Name = sqldata[ncol + 2] ? sqldata[ncol + 2] : "";
        data.Type = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
        data.PlatId = sqldata[ncol + 4] ? sqldata[ncol + 4] : "0";
        data.XLength = atof(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0.0");
        data.YLength = atof(sqldata[ncol + 6] ? sqldata[ncol + 6] : "0.0");
        data.LaneNumber = atoi(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0");
        data.Latitude = sqldata[ncol + 8] ? sqldata[ncol + 8] : "";
        data.Longitude = sqldata[ncol + 9] ? sqldata[ncol + 9] : "";
        data.FlagEast = atoi(sqldata[ncol + 10] ? sqldata[ncol + 10] : "0");
        data.FlagSouth = atoi(sqldata[ncol + 11] ? sqldata[ncol + 11] : "0");
        data.FlagWest = atoi(sqldata[ncol + 12] ? sqldata[ncol + 12] : "0");
        data.FlagNorth = atoi(sqldata[ncol + 13] ? sqldata[ncol + 13] : "0");
        data.DeltaXEast = atof(sqldata[ncol + 14] ? sqldata[ncol + 14] : "0.0");
        data.DeltaYEast = atof(sqldata[ncol + 15] ? sqldata[ncol + 15] : "0.0");
        data.DeltaXSouth = atof(sqldata[ncol + 16] ? sqldata[ncol + 16] : "0.0");
        data.DeltaYSouth = atof(sqldata[ncol + 17] ? sqldata[ncol + 17] : "0.0");
        data.DeltaXWest = atof(sqldata[ncol + 18] ? sqldata[ncol + 18] : "0.0");
        data.DeltaYWest = atof(sqldata[ncol + 19] ? sqldata[ncol + 19] : "0.0");
        data.DeltaXNorth = atof(sqldata[ncol + 20] ? sqldata[ncol + 20] : "0.0");
        data.DeltaYNorth = atof(sqldata[ncol + 21] ? sqldata[ncol + 21] : "0.0");
        data.WidthX = atof(sqldata[ncol + 22] ? sqldata[ncol + 22] : "0.0");
        data.WidthY = atof(sqldata[ncol + 23] ? sqldata[ncol + 23] : "0.0");
    } else {
        LOG(ERROR) << "db select count err:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int delete_fusion_para_set() {
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from fusion_para_set");

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int insert_fusion_para_set(DB_Fusion_Para_Set_T data) {
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "insert into fusion_para_set("
                           "IntersectionAreaPoint1X,IntersectionAreaPoint1Y,IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
                           "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,IntersectionAreaPoint4X,IntersectionAreaPoint4Y) "
                           "values(%f,%f,%f,%f,%f,%f,%f,%f)",
             data.IntersectionAreaPoint1X, data.IntersectionAreaPoint1Y, data.IntersectionAreaPoint2X,
             data.IntersectionAreaPoint2Y, data.IntersectionAreaPoint3X, data.IntersectionAreaPoint3Y,
             data.IntersectionAreaPoint4X, data.IntersectionAreaPoint4Y);

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int get_fusion_para_set(DB_Fusion_Para_Set_T &data) {
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
    ret = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    if (nrow == 1) {
        data.IntersectionAreaPoint1X = atof(sqldata[ncol + 1] ? sqldata[ncol + 1] : "0.0");
        data.IntersectionAreaPoint1Y = atof(sqldata[ncol + 2] ? sqldata[ncol + 2] : "0.0");
        data.IntersectionAreaPoint2X = atof(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0.0");
        data.IntersectionAreaPoint2Y = atof(sqldata[ncol + 4] ? sqldata[ncol + 4] : "0.0");
        data.IntersectionAreaPoint3X = atof(sqldata[ncol + 5] ? sqldata[ncol + 5] : "0.0");
        data.IntersectionAreaPoint3Y = atof(sqldata[ncol + 6] ? sqldata[ncol + 6] : "0.0");
        data.IntersectionAreaPoint4X = atof(sqldata[ncol + 7] ? sqldata[ncol + 7] : "0.0");
        data.IntersectionAreaPoint4Y = atof(sqldata[ncol + 8] ? sqldata[ncol + 8] : "0.0");
    } else {
        LOG(ERROR) << "db table select count err:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);
    free(sqlstr);
    return 0;
}

int delete_associated_equip() {
    int ret = 0;
    char *sqlstr = new char[1024];
    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "delete from associated_equip");

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    delete[] sqlstr;
    return 0;
}

int insert_associated_equip(DB_Associated_Equip_T data) {
    int ret = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024 - 1, "insert into associated_equip("
                               "EquipType,EquipCode) values (%d,'%s')",
             data.EquipType, data.EquipCode.c_str());

    ret = dbFileExecSql(roadeside_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int get_associated_equip(std::vector<DB_Associated_Equip_T> &data) {
    int ret = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select id,EquipType,EquipCode from associated_equip");
    ret = dbFileExecSqlTable(roadeside_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return -1;
    }
    data.clear();
    DB_Associated_Equip_T tmp_data;
    for (int i = 0; i < nrow; i++) {
        int index = ncol * (i + 1);
        tmp_data.EquipType = atoi(sqldata[index + 1] ? sqldata[index + 1] : "0");
        tmp_data.EquipCode = sqldata[index + 2] ? sqldata[index + 2] : "";
        data.push_back(tmp_data);
    }

    delete[] sqlstr;
    dbFreeTable(sqldata);
    return 0;
}

int
db_parking_lot_get_cloud_addr_from_factory(std::string &server_path, int &server_port, std::string &file_server_path,
                                           int &file_server_port) {
    int rtn = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select CloudServerPath,TransferServicePath,CloudServerPort,FileServicePort from TB_ParkingLot "
                    "where ID=(select MIN(ID) from TB_ParkingLot)");
    rtn = dbFileExecSqlTable(factory_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    if (nrow == 1) {
        server_path = sqldata[ncol + 0] ? sqldata[ncol + 0] : "";
        file_server_path = sqldata[ncol + 1] ? sqldata[ncol + 1] : "";
        server_port = atoi(sqldata[ncol + 2] ? sqldata[ncol + 2] : "0");
        file_server_port = atoi(sqldata[ncol + 3] ? sqldata[ncol + 3] : "0");
    } else {
        LOG(ERROR) << "db table select count err sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);

    return 0;
}

int bin_parking_lot_update_eocpath(const char *serverPath, int serverPort, const char *filePath, int filePort) {
    int rtn = 0;
    char *sqlstr = new char[1024];

    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr, 1024, "update TB_ParkingLot set CloudServerPath='%s',"
                           "TransferServicePath='%s',CloudServerPort=%d,FileServicePort=%d",
             serverPath, filePath, serverPort, filePort);

    rtn = dbFileExecSql(factory_parking_db.path.c_str(), sqlstr, NULL, NULL);
    if (rtn < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }

    delete[] sqlstr;
    return 0;
}

int db_factory_get_uname(std::string &uname) {
    int rtn = 0;
    char *sqlstr = new char[1024];
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    memset(sqlstr, 0, 1024);
    sprintf(sqlstr, "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)");
    rtn = dbFileExecSqlTable(cl_parking_db.path.c_str(), sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0) {
        LOG(ERROR) << "db sql:" << sqlstr << "fail";
        delete[] sqlstr;
        return -1;
    }
    if (nrow == 1) {
        uname = sqldata[1] ? sqldata[1] : "";
    } else {
        LOG(ERROR) << "db table select count err sql:" << sqlstr;
        delete[] sqlstr;
        dbFreeTable(sqldata);
        return 1;
    }
    delete[] sqlstr;
    dbFreeTable(sqldata);

    return 0;
}
