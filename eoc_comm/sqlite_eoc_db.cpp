
#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>

#include "sqlite3.h"
#include "logger.h"
#include "sqlite_api.h"
#include "sqlite_eoc_db.h"
#include "db_tool.hpp"


typedef struct Database_t{
    pthread_mutex_t db_mutex;
    char db_file_path[256];
    char db_version[32];
}Database;
//V_1_0_0 初始建立
Database roadeside_parking_db = {PTHREAD_MUTEX_INITIALIZER, "./eoc_configure.db", "V_1_0_0"}; 

Database cl_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/CLParking.db", "V1_0"};
Database factory_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/RoadsideParking.db", "V1_0"};

//核心板基础配置
static DBT_Table base_set_table[] = {
    {"base_set", "DevIndex", "INTERGE"},
    {"base_set", "City", "TEXT"},
    {"base_set", "IsUploadToPlatform", "INTERGE"},
    {"base_set", "Is4Gmodel", "INTERGE"},
    {"base_set", "IsIllegalCapture", "INTERGE"},
    {"base_set", "IsPrintIntersectionName", "INTERGE"},
    {"base_set", "FilesServicePath", "TEXT"},
    {"base_set", "FilesServicePort", "INTERGE"},
    {"base_set", "MainDNS", "TEXT"},
    {"base_set", "AlternateDNS", "TEXT"},
    {"base_set", "PlatformTcpPath", "TEXT"},
    {"base_set", "PlatformTcpPort", "INTERGE"},
    {"base_set", "PlatformHttpPath", "TEXT"},
    {"base_set", "PlatformHttpPort", "INTERGE"},
    {"base_set", "SignalMachinePath", "TEXT"},
    {"base_set", "SignalMachinePort", "INTERGE"},
    {"base_set", "IsUseSignalMachine", "INTERGE"},
    {"base_set", "NtpServerPath", "TEXT"},
    {"base_set", "FusionMainboardIp", "TEXT"},
    {"base_set", "FusionMainboardPort", "INTERGE"},
    {"base_set", "IllegalPlatformAddress", "TEXT"},
    {"base_set", "Remarks", "TEXT"}};

//所属路口信息
static DBT_Table belong_intersection_table[] = {
    {"belong_intersection", "Guid", "TEXT"},
    {"belong_intersection", "Name", "TEXT"},
    {"belong_intersection", "Type", "INTERGE"},
    {"belong_intersection", "PlatId", "TEXT"},
    {"belong_intersection", "XLength", "REAL"},
    {"belong_intersection", "YLength", "REAL"},
    {"belong_intersection", "LaneNumber", "INTERGE"},
    {"belong_intersection", "Latitude", "TEXT"},
    {"belong_intersection", "Longitude", "TEXT"},

    {"belong_intersection", "FlagEast", "INTERGE"},
    {"belong_intersection", "FlagSouth", "INTERGE"},
    {"belong_intersection", "FlagWest", "INTERGE"},
    {"belong_intersection", "FlagNorth", "INTERGE"},
    {"belong_intersection", "DeltaXEast", "REAL"},
    {"belong_intersection", "DeltaYEast", "REAL"},
    {"belong_intersection", "DeltaXSouth", "REAL"},
    {"belong_intersection", "DeltaYSouth", "REAL"},
    {"belong_intersection", "DeltaXWest", "REAL"},
    {"belong_intersection", "DeltaYWest", "REAL"},
    {"belong_intersection", "DeltaXNorth", "REAL"},
    {"belong_intersection", "DeltaYNorth", "REAL"},
    {"belong_intersection", "WidthX", "REAL"},
    {"belong_intersection", "WidthY", "REAL"}};

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

/****表字段检查配置******/
typedef struct
{
    char column_name[64];
    char data_type[64];
}DB_Table_Configure;
//最初设置非空属性的字段不检测，仅检测后期添加字段
//DB_Table_Configure TB_RecognitionArea_table[] = {{"RotateAngle",    "INTEGER"}};

int check_table_column(string tab_name, DB_Table_Configure tab_column[], int check_num)
{
    int rtn = 0;
    char sqlstr[1024] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    int icol = 0;

    for(icol=0; icol<check_num; icol++)
    {
        nrow = 2;
        ncol = 0;
        memset(sqlstr, 0x0, 1024);
        sprintf(sqlstr, "select * from sqlite_master where name='%s' and sql like '%%%s%%';", 
                        tab_name.c_str(), tab_column[icol].column_name);
        rtn = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
        if (rtn < 0)
        {
            DBG("db_file_exec_sql_table:%s err.",sqlstr);
            return -1;
        }
        if(nrow == 0)
        {
            //添加字段
            memset(sqlstr, 0x0, 1024);
            sprintf(sqlstr, "alter table %s add %s %s", tab_name.c_str(), tab_column[icol].column_name, tab_column[icol].data_type);
            rtn = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
            if(rtn < 0)
            {
                DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
            //  return -1;
            }
            else
            {
                DBG("add column OK! sql:%s", sqlstr);
            }
        }
        
        sqlite3_free_table_safe(sqldata);
    }

    return 0;
}
/*
 * eoc配置表初始化，如果表不存在则创建表
 *
 * 0：成功
 * -1：失败
 * */
int eoc_db_table_init()
{
    int ret;
    DBG("using database file is:%s[%s]", roadeside_parking_db.db_file_path, roadeside_parking_db.db_version);
#if 0
    ret = dbt_mkdir_from_path(roadeside_parking_db.db_file_path);
    if(rtn != 0){
        ERR("%s dbt_mkdir_from_path:%s Err", __FUNCTION__, roadeside_parking_db.db_file_path);
    }
#endif
    int count = 1;
    char *sqlstr = NULL;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    char cur_db_version[16] = {0};
    memset(cur_db_version, 0x0, 16);

    sqlstr = (char *)malloc(2048);
    if (sqlstr == NULL)
    {
        ERR("申请内存错误");
        return -1;
    }
    memset(sqlstr, 0x0, 1024);
    sprintf(sqlstr, "select tbl_name from sqlite_master where type='table'");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        free(sqlstr);
        return -1;
    }
    for(int i=0; i<nrow; i++)
    {       
        if(strstr(sqldata[i+1], "V_"))
        {
            strcpy(cur_db_version, sqldata[i+1]);
            if(strcmp(cur_db_version, roadeside_parking_db.db_version) == 0){
                count = 0;
            }
            break;
        }
    }
    sqlite3_free_table_safe(sqldata);

    if(count != 0)
    {
        DBG("check db_version(%s) from %s failed, local db_ver(%s)", roadeside_parking_db.db_version, roadeside_parking_db.db_file_path, cur_db_version);
        
        //判断表是否存在创建表，并检查补充字段
        memset(sqlstr, 0x0, 1024);
        snprintf(sqlstr,1024,"create table IF NOT EXISTS conf_version("
            "Id INTEGER PRIMARY KEY NOT NULL,"
            "version TEXT NOT NULL,"
            "time       TEXT NOT NULL)");
        ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
        if(ret < 0)
        {
            DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
            free(sqlstr);
            return -1;
        }
        //删除数据版本,登录成功后会主动要一次配置
        memset(sqlstr, 0x0, 1024);
        snprintf(sqlstr,1024,"delete from conf_version");
        ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
        if(ret < 0)
        {
            DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
            free(sqlstr);
            return -1;
        }

        //基础配置表
        ret = dbt_check(roadeside_parking_db.db_file_path, base_set_table, sizeof(base_set_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check base_set_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //所属路口信息表
        ret = dbt_check(roadeside_parking_db.db_file_path, belong_intersection_table, sizeof(belong_intersection_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check belong_intersection_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //融合参数设置表
        ret = dbt_check(roadeside_parking_db.db_file_path, fusion_para_set_table, sizeof(fusion_para_set_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check camera_info_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //关联设备表
        ret = dbt_check(roadeside_parking_db.db_file_path, associated_equip_table, sizeof(associated_equip_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check camera_detail_set_table Err,return -1", __FUNCTION__);
            return -1;
        }

        //更新为新版本数据库版本表
        memset(sqlstr, 0x0, 1024);
        if(cur_db_version[0] == 0x0){
            snprintf(sqlstr,1024,"create table IF NOT EXISTS %s(id INTEGER PRIMARY KEY NOT NULL)",roadeside_parking_db.db_version);
        }else{
            snprintf(sqlstr,1024,"alter table %s rename to %s", cur_db_version, roadeside_parking_db.db_version);
        }
        ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
        if(ret < 0)
        {
            DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
            free(sqlstr);
            return -1;
        }
    }
    else
    {
        DBG("check db_version(%s) from %s succeed", roadeside_parking_db.db_version, roadeside_parking_db.db_file_path);
    }
    free(sqlstr);
    return 0;
}

int db_version_delete()
{
    int ret = 0;
    char sqlstr[256] = {0};

    sprintf(sqlstr,"delete from conf_version");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_version_insert(DB_Conf_Data_Version &data)
{
    int ret = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr,1024,"insert into conf_version(version,time) values('%s','%s')",
            data.version.c_str(), data.time.c_str());
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;
}
int db_version_get(string &conf_version)
{
    int ret = 0;
    char sqlstr[256] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sprintf(sqlstr, "select version from conf_version where id=(select MIN(id) from conf_version)");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        conf_version = sqldata[1]? sqldata[1]: "";
    }
    else
    {
        conf_version = "";
        DBG("conf_version tabal date select count err");
        sqlite3_free_table_safe(sqldata);
        return 1;
    }
    sqlite3_free_table_safe(sqldata);

    return 0;
}

//基础配置信息
int db_base_set_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from base_set");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_base_set_insert(DB_Base_Set_T &data)
{
    int ret = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr,1024,"insert into base_set("
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
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;
}
int db_base_set_get(DB_Base_Set_T &data)
{
    int ret = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }

    sprintf(sqlstr, "select id,DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,Remarks,"
            "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
            "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
            "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
            "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress "
            "from base_set order by id desc limit 1");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        data.Index = atoi(sqldata[ncol+1]? sqldata[ncol+1]: "0");
        data.City = sqldata[ncol+2]? sqldata[ncol+2]: "";
        data.IsUploadToPlatform = atoi(sqldata[ncol+3]? sqldata[ncol+3]: "0");
        data.Is4Gmodel = atoi(sqldata[ncol+4]? sqldata[ncol+4]: "0");
        data.IsIllegalCapture = atoi(sqldata[ncol+5]? sqldata[ncol+5]: "0");
        data.Remarks = sqldata[ncol+6]? sqldata[ncol+6]: "";
        data.IsPrintIntersectionName = atoi(sqldata[ncol+7]? sqldata[ncol+7]: "0");
        data.FilesServicePath = sqldata[ncol+8]? sqldata[ncol+8]: "";
        data.FilesServicePort = atoi(sqldata[ncol+9]? sqldata[ncol+9]: "0");
        data.MainDNS = sqldata[ncol+10]? sqldata[ncol+10]: "";
        data.AlternateDNS = sqldata[ncol+11]? sqldata[ncol+11]: "";
        data.PlatformTcpPath = sqldata[ncol+12]? sqldata[ncol+12]: "";
        data.PlatformTcpPort = atoi(sqldata[ncol+13]? sqldata[ncol+13]: "0");
        data.PlatformHttpPath = sqldata[ncol+14]? sqldata[ncol+14]: "";
        data.PlatformHttpPort = atoi(sqldata[ncol+15]? sqldata[ncol+15]: "0");
        data.SignalMachinePath = sqldata[ncol+16]? sqldata[ncol+16]: "";
        data.SignalMachinePort = atoi(sqldata[ncol+17]? sqldata[ncol+17]: "0");
        data.IsUseSignalMachine = atoi(sqldata[ncol+18]? sqldata[ncol+18]: "0");
        data.NtpServerPath = sqldata[ncol+19]? sqldata[ncol+19]: "";
        data.FusionMainboardIp = sqldata[ncol+20]? sqldata[ncol+20]: "";
        data.FusionMainboardPort = atoi(sqldata[ncol+21]? sqldata[ncol+21]: "0");
        data.IllegalPlatformAddress = sqldata[ncol+22]? sqldata[ncol+22]:"";
    }
    else
    {
        DBG("base_set tabal date select count err");
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return 1;
    }
    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}

//所属路口信息
int db_belong_intersection_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from belong_intersection");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_belong_intersection_insert(DB_Intersection_T &data)
{
    int ret = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(2048);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 2048);
    snprintf(sqlstr,2048,"insert into belong_intersection("
            "Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
            "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
            "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY) "
            "values('%s','%s',%d,'%s',%f,%f,%d,'%s','%s',%d,%d,%d,%d,%f,%f,%f,%f,%f,"
            "%f,%f,%f,%f,%f)",
            data.Guid.c_str(), data.Name.c_str(), data.Type, data.PlatId.c_str(), 
            data.XLength, data.YLength, data.LaneNumber, 
            data.Latitude.c_str(), data.Longitude.c_str(), data.FlagEast, data.FlagSouth, data.FlagWest, data.FlagNorth, 
            data.DeltaXEast, data.DeltaYEast, data.DeltaXSouth, data.DeltaYSouth, 
            data.DeltaXWest, data.DeltaYWest, data.DeltaXNorth, data.DeltaYNorth, data.WidthX, data.WidthY);
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;
}
int db_belong_intersection_get(DB_Intersection_T &data)
{
    int ret = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }

    sprintf(sqlstr, "select id,Guid,Name,Type,PlatId,XLength,YLength,LaneNumber,Latitude,Longitude,"
            "FlagEast,FlagSouth,FlagWest,FlagNorth,DeltaXEast,DeltaYEast,DeltaXSouth,DeltaYSouth,"
            "DeltaXWest,DeltaYWest,DeltaXNorth,DeltaYNorth,WidthX,WidthY "
            "from belong_intersection order by id desc limit 1");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        data.Guid = sqldata[ncol+1]? sqldata[ncol+1]: "";
        data.Name = sqldata[ncol+2]? sqldata[ncol+2]: "";
        data.Type = atoi(sqldata[ncol+3]? sqldata[ncol+3]: "0");
        data.PlatId = sqldata[ncol+4]? sqldata[ncol+4]: "0";
        data.XLength = atof(sqldata[ncol+5]? sqldata[ncol+5]: "0.0");
        data.YLength = atof(sqldata[ncol+6]? sqldata[ncol+6]: "0.0");
        data.LaneNumber = atoi(sqldata[ncol+7]? sqldata[ncol+7]: "0");
        data.Latitude = sqldata[ncol+8]? sqldata[ncol+8]: "";
        data.Longitude = sqldata[ncol+9]? sqldata[ncol+9]: "";
        data.FlagEast = atoi(sqldata[ncol+10]? sqldata[ncol+10]: "0");
        data.FlagSouth = atoi(sqldata[ncol+11]? sqldata[ncol+11]: "0");
        data.FlagWest = atoi(sqldata[ncol+12]? sqldata[ncol+12]: "0");
        data.FlagNorth = atoi(sqldata[ncol+13]? sqldata[ncol+13]: "0");
        data.DeltaXEast = atof(sqldata[ncol+14]? sqldata[ncol+14]: "0.0");
        data.DeltaYEast = atof(sqldata[ncol+15]? sqldata[ncol+15]: "0.0");
        data.DeltaXSouth = atof(sqldata[ncol+16]? sqldata[ncol+16]: "0.0");
        data.DeltaYSouth = atof(sqldata[ncol+17]? sqldata[ncol+17]: "0.0");
        data.DeltaXWest = atof(sqldata[ncol+18]? sqldata[ncol+18]: "0.0");
        data.DeltaYWest = atof(sqldata[ncol+19]? sqldata[ncol+19]: "0.0");
        data.DeltaXNorth = atof(sqldata[ncol+20]? sqldata[ncol+20]: "0.0");
        data.DeltaYNorth = atof(sqldata[ncol+21]? sqldata[ncol+21]: "0.0");
        data.WidthX = atof(sqldata[ncol+22]? sqldata[ncol+22]: "0.0");
        data.WidthY = atof(sqldata[ncol+23]? sqldata[ncol+23]: "0.0");
    }
    else
    {
        DBG("tabal date select count err");
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return 1;
    }
    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}

//融合参数设置
int db_fusion_para_set_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from fusion_para_set");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_fusion_para_set_insert(DB_Fusion_Para_Set_T &data)
{
    int ret = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr,1024,"insert into fusion_para_set("
            "IntersectionAreaPoint1X,IntersectionAreaPoint1Y,IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
            "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,IntersectionAreaPoint4X,IntersectionAreaPoint4Y) "
            "values(%f,%f,%f,%f,%f,%f,%f,%f)",
            data.IntersectionAreaPoint1X, data.IntersectionAreaPoint1Y, data.IntersectionAreaPoint2X, 
            data.IntersectionAreaPoint2Y, data.IntersectionAreaPoint3X, data.IntersectionAreaPoint3Y, 
            data.IntersectionAreaPoint4X, data.IntersectionAreaPoint4Y);
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;
}
int db_fusion_para_set_get(DB_Fusion_Para_Set_T &data)
{
    int ret = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }

    sprintf(sqlstr, "select id,IntersectionAreaPoint1X,IntersectionAreaPoint1Y,IntersectionAreaPoint2X,IntersectionAreaPoint2Y,"
            "IntersectionAreaPoint3X,IntersectionAreaPoint3Y,IntersectionAreaPoint4X,IntersectionAreaPoint4Y "
            "from fusion_para_set order by id desc limit 1");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        data.IntersectionAreaPoint1X = atof(sqldata[ncol+1]? sqldata[ncol+1]: "0.0");
        data.IntersectionAreaPoint1Y = atof(sqldata[ncol+2]? sqldata[ncol+2]: "0.0");
        data.IntersectionAreaPoint2X = atof(sqldata[ncol+3]? sqldata[ncol+3]: "0.0");
        data.IntersectionAreaPoint2Y = atof(sqldata[ncol+4]? sqldata[ncol+4]: "0.0");
        data.IntersectionAreaPoint3X = atof(sqldata[ncol+5]? sqldata[ncol+5]: "0.0");
        data.IntersectionAreaPoint3Y = atof(sqldata[ncol+6]? sqldata[ncol+6]: "0.0");
        data.IntersectionAreaPoint4X = atof(sqldata[ncol+7]? sqldata[ncol+7]: "0.0");
        data.IntersectionAreaPoint4Y = atof(sqldata[ncol+8]? sqldata[ncol+8]: "0.0");
    }
    else
    {
        DBG("tabal date select count err");
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return 1;
    }
    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}

//关联设备
int db_associated_equip_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from associated_equip");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_associated_equip_insert(DB_Associated_Equip_T &data)
{
    int ret = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(512);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 512);
    snprintf(sqlstr,512,"insert into associated_equip("
            "EquipType,EquipCode) values (%d,'%s')",
            data.EquipType, data.EquipCode.c_str());
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;
}
int db_associated_equip_get(vector <DB_Associated_Equip_T> &data)
{
    int ret = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    sqlstr = (char *)malloc(64);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }

    sprintf(sqlstr, "select id,EquipType,EquipCode from associated_equip");
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    data.clear();
    int i = 0;
    DB_Associated_Equip_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.EquipType = atoi(sqldata[index+1]? sqldata[index+1]: "0");
        tmp_data.EquipCode = sqldata[index+2]? sqldata[index+2]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}




int db_parking_lot_get_cloud_addr_from_factory(string &server_path, int* server_port, string &file_server_path, int* file_server_port)
{
	int rtn = 0;
	char sqlstr[256] = {0};
	char **sqldata;
    int nrow = 0;
    int ncol = 0;

	sprintf(sqlstr, "select CloudServerPath,TransferServicePath,CloudServerPort,FileServicePort from TB_ParkingLot "
            "where ID=(select MIN(ID) from TB_ParkingLot)");
	rtn = db_file_exec_sql_table(factory_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.\n",sqlstr);
        return -1;
    }
	if(nrow == 1)
	{
		server_path = sqldata[ncol+0]? sqldata[ncol+0]: "";
		file_server_path = sqldata[ncol+1]? sqldata[ncol+1]: "";
		*server_port = atoi(sqldata[ncol+2]? sqldata[ncol+2]: "0");
		*file_server_port = atoi(sqldata[ncol+3]? sqldata[ncol+3]: "0");
	}
	else
	{
	    DBG("TB_ParkingLot tabal date select count err\n");
		sqlite3_free_table_safe(sqldata);
	    return 1;
	}
	sqlite3_free_table_safe(sqldata);

	return 0;
}

//更新~/bin/RoadsideParking.db下eoc服务器地址和文件服务器地址
int bin_parking_lot_update_eocpath(const char *serverPath, int serverPort, const char *filePath, int filePort)
{
    int rtn = 0;
    char *sqlstr;

    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 1024);
    snprintf(sqlstr,1024,"update TB_ParkingLot set CloudServerPath='%s',"
            "TransferServicePath='%s',CloudServerPort=%d,FileServicePort=%d",
            serverPath, filePath, serverPort, filePort);
    
    rtn = db_file_exec_sql(factory_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(rtn < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s\n", sqlstr);
        free(sqlstr);
        return -1;
    }

    free(sqlstr);
    return 0;

}

/***取id最大一条数据的用户名***/
int db_factory_get_uname(string &uname)
{
    //ceshi
//    uname = std::string("10200440-1245-403b-be35-223bcdd41fd4");
//    return 0;

    int rtn = 0;
    char sqlstr[256] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sprintf(sqlstr, "select UName from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)");
    rtn = db_file_exec_sql_table(cl_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        uname = sqldata[1]? sqldata[1]: "";
    }
    else
    {
        DBG("CL_ParkingArea tabal date select count err");
        sqlite3_free_table_safe(sqldata);
        return 1;
    }
    sqlite3_free_table_safe(sqldata);

    return 0;
}



