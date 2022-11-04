
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
//V_1_0_2对应通信协议V1.0.0，添加路口参数设置
//V_1_0_3对应通信协议V1.0.0，雷达设置参数修改：添加ip、端口号，删除是否启用
//V_1_0_4对应通信协议V1.0.1
Database roadeside_parking_db = {PTHREAD_MUTEX_INITIALIZER, "./eoc_configure.db", "V_1_0_4"}; 

Database cl_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/CLParking.db", "V1_0"};
Database factory_parking_db = {PTHREAD_MUTEX_INITIALIZER, HOME_PATH"/bin/RoadsideParking.db", "V1_0"};

//核心板基础配置
static DBT_Table base_set_table[] = {
    {"base_set", "DevIndex", "INTERGE"},
    {"base_set", "City", "TEXT"},
    {"base_set", "IsUploadToPlatform", "INTERGE"},
    {"base_set", "Is4Gmodel", "INTERGE"},
    {"base_set", "IsIllegalCapture", "INTERGE"},
    {"base_set", "PlateDefault", "TEXT"},
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
    {"base_set", "CCK1Guid", "TEXT"},
    {"base_set", "CCK1IP", "TEXT"},
    {"base_set", "ErvPlatId", "TEXT"}};

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

//相机信息
static DBT_Table camera_info_table[] = {
    {"camera_info", "Guid", "TEXT"},
    {"camera_info", "Ip", "TEXT"},
    {"camera_info", "Name", "TEXT"},
    {"camera_info", "AccessMode", "INTERGE"},
    {"camera_info", "Type", "INTERGE"},
    {"camera_info", "CheckRange", "INTERGE"},
    {"camera_info", "Direction", "INTERGE"},
    {"camera_info", "PixelType", "INTERGE"},
    {"camera_info", "Delay", "INTERGE"},
    {"camera_info", "IsEnable", "INTERGE"}};

//雷达信息
static DBT_Table radar_info_table[] = {
    {"radar_info", "Guid", "TEXT"},
    {"radar_info", "Name", "TEXT"},
    {"radar_info", "Ip", "TEXT"},
    {"radar_info", "Port", "INTERGE"},
    {"radar_info", "XThreshold", "REAL"},
    {"radar_info", "YThreshold", "REAL"},
    {"radar_info", "XCoordinate", "REAL"},
    {"radar_info", "YCoordinate", "REAL"},
    {"radar_info", "XRadarCoordinate", "REAL"},
    {"radar_info", "YRadarCoordinate", "REAL"},
    {"radar_info", "HRadarCoordinate", "REAL"},
    {"radar_info", "CorrectiveAngle", "REAL"},
    {"radar_info", "XOffset", "REAL"},
    {"radar_info", "YOffset", "REAL"},
    {"radar_info", "CheckDirection", "INTERGE"}};

//相机详细配置
static DBT_Table camera_detail_set_table[] = {
    {"camera_detail_set", "Guid", "TEXT"},
    {"camera_detail_set", "DataVersion", "TEXT"},
    
    {"camera_detail_set", "Brightness", "INTERGE"},
    {"camera_detail_set", "Contrast", "INTERGE"},
    {"camera_detail_set", "Saturation", "INTERGE"},
    {"camera_detail_set", "Acutance", "INTERGE"},
    {"camera_detail_set", "MaximumGain", "INTERGE"},
    {"camera_detail_set", "ExposureTime", "INTERGE"},

    {"camera_detail_set", "DigitalNoiseReduction", "INTERGE"},
    {"camera_detail_set", "DigitalNoiseReductionLevel", "INTERGE"},

    {"camera_detail_set", "WideDynamic", "INTERGE"},
    {"camera_detail_set", "WideDynamicLevel", "INTERGE"},

    {"camera_detail_set", "BitStreamType", "INTERGE"},
    {"camera_detail_set", "Resolution", "INTERGE"},
    {"camera_detail_set", "Fps", "INTERGE"},
    {"camera_detail_set", "CodecType", "INTERGE"},
    {"camera_detail_set", "ImageQuality", "INTERGE"},
    {"camera_detail_set", "BitRateUpper", "INTERGE"},

    {"camera_detail_set", "IsShowName", "INTERGE"},
    {"camera_detail_set", "IsShowDate", "INTERGE"},
    {"camera_detail_set", "IsShowWeek", "INTERGE"},
    {"camera_detail_set", "TimeFormat", "INTERGE"},
    {"camera_detail_set", "DateFormat", "TEXT"},
    {"camera_detail_set", "ChannelName", "TEXT"},
    {"camera_detail_set", "FontSize", "INTERGE"},
    {"camera_detail_set", "FontColor", "TEXT"},
    {"camera_detail_set", "Transparency", "INTERGE"},
    {"camera_detail_set", "Flicker", "INTERGE"}};

//车道信息
static DBT_Table lane_info_table[] = {
    {"lane_info", "Guid", "TEXT"},
    {"lane_info", "Name", "TEXT"},    //车道名称
    {"lane_info", "SerialNumber", "INTERGE"},       // 车道号
    {"lane_info", "DrivingDirection", "INTERGE"},   //行驶方向：1=东，2=南，3=西，4=北
    {"lane_info", "Type", "INTERGE"},               // 车道类型：1=直行车道，2=左转车道，3=右转车道，4=掉头车道，5=直行-左转车道，
                                //6=直行右转车道，7=直行-掉头车道，8=左转-掉头车道，9=直行-左转-掉头车道
    {"lane_info", "Length", "REAL"},   //长度
    {"lane_info", "Width", "REAL"},    //宽度
    {"lane_info", "IsEnable", "INTERGE"},   //是否启用 1=启用，2=不启用
    {"lane_info", "CameraName", "TEXT"}};

//************识别相机
//车道区域识别区点
static DBT_Table lane_identif_area_point_table[] = {
    {"lane_identif_area_point", "IdentifAreaGuid", "TEXT"}, //所属识别区guid
    {"lane_identif_area_point", "SerialNumber", "INTERGE"},   //点序号
    {"lane_identif_area_point", "XPixel", "REAL"},       //像素横坐标x值
    {"lane_identif_area_point", "YPixel", "REAL"}};       //像素横坐标x值
//车道区域识别区
static DBT_Table lane_identif_area_table[] = {
    {"lane_identif_area", "LaneGuid", "TEXT"},      //所属车道区域guid
    {"lane_identif_area", "IdentifAreaGuid", "TEXT"},   //识别区guid
    {"lane_identif_area", "Type", "INTERGE"},       //1=车牌识别区，2=车头识别区，3=车尾识别区
    {"lane_identif_area", "CameraGuid", "TEXT"}};           //所属相机guid
//车道区域车道线
static DBT_Table lane_line_table[] = {
    {"lane_line", "LaneGuid", "TEXT"},      //所属车道区域guid
    {"lane_line", "LaneLineGuid", "TEXT"},   //车道线guid
    {"lane_line", "Type", "INTERGE"},           // 车道线类型：1=虚线，2=实线，3=双实线，4=双虚线，5=上虚下实线，6=下虚上实线
    {"lane_line", "Color", "INTERGE"},          // 车道线颜色：1=白色，2=黄色
    {"lane_line", "CoordinateSet", "TEXT"},    //车道线点坐标集合
    {"lane_line", "CameraGuid", "TEXT"}};   //所属相机guid

//****************检测相机
//防抖动区
static DBT_Table detect_shake_point_table[] = {
    {"detect_shake_point", "ShakeAreaGuid", "TEXT"},    //所属防抖区guid
    {"detect_shake_point", "SerialNumber", "INTERGE"},  //点序号
    {"detect_shake_point", "XCoordinate", "REAL"},      //相对路口x坐标
    {"detect_shake_point", "YCoordinate", "REAL"},     //相对路口y坐标
    {"detect_shake_point", "XPixel", "REAL"},       //像素横坐标x值
    {"detect_shake_point", "YPixel", "REAL"}};       //像素横坐标x值
static DBT_Table detect_shake_area_table[] = {
    {"detect_shake_area", "Guid", "TEXT"},  //guid
    {"detect_shake_area", "XOffset", "REAL"},   //抖动位偏移x
    {"detect_shake_area", "YOffset", "REAL"},  //抖动位偏移Y
    {"detect_shake_area", "CameraGuid", "TEXT"}};  //所属相机guid
//融合区
static DBT_Table detect_fusion_point_table[] = {
    {"detect_fusion_point", "FusionAreaGuid", "TEXT"},  //所属融合区guid
    {"detect_fusion_point", "SerialNumber", "INTERGE"}, //点序号
    {"detect_fusion_point", "XCoordinate", "REAL"},     //相对路口x坐标
    {"detect_fusion_point", "YCoordinate", "REAL"},     //相对路口y坐标
    {"detect_fusion_point", "XPixel", "REAL"},          //像素横坐标x值
    {"detect_fusion_point", "YPixel", "REAL"}};         //像素横坐标x值
static DBT_Table detect_fusion_area_table[] = {
    {"detect_fusion_area", "Guid", "TEXT"},      //guid
    {"detect_fusion_area", "PerspectiveTransFa00", "REAL"},   //系数矩阵-透视变换用系数矩阵，3*3矩阵
    {"detect_fusion_area", "PerspectiveTransFa01", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa02", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa10", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa11", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa12", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa20", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa21", "REAL"},
    {"detect_fusion_area", "PerspectiveTransFa22", "REAL"},
    {"detect_fusion_area", "XDistance", "REAL"},    //变化实测距离X-透视变换最左边的直线与Y轴之间的实际测量距离
    {"detect_fusion_area", "YDistance", "REAL"},    //变化实测距离Y-透视变换最下面的直线与X轴之间的世界测量距离
    {"detect_fusion_area", "HDistance", "REAL"},    // 变换实测距离H-透视变化图像高度
    {"detect_fusion_area", "MPPW", "REAL"},     //宽度米像素
    {"detect_fusion_area", "MPPH", "REAL"},     //高度米像素
    {"detect_fusion_area", "CameraGuid", "TEXT"}};  //所属相机guid



/****表字段检查配置******/
typedef struct
{
    char column_name[64];
    char data_type[64];
}DB_Table_Configure;
//最初设置非空属性的字段不检测，仅检测后期添加字段
DB_Table_Configure TB_RecognitionArea_table[] = {{"RotateAngle",    "INTEGER"}};


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
        //相机信息表
        ret = dbt_check(roadeside_parking_db.db_file_path, camera_info_table, sizeof(camera_info_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check camera_info_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //雷达信息表
        ret = dbt_check(roadeside_parking_db.db_file_path, radar_info_table, sizeof(radar_info_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check radar_info_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //相机详细配置表
        ret = dbt_check(roadeside_parking_db.db_file_path, camera_detail_set_table, sizeof(camera_detail_set_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check camera_detail_set_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //车道信息
        ret = dbt_check(roadeside_parking_db.db_file_path, lane_info_table, sizeof(lane_info_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check lane_info_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //车道区域识别区点
        ret = dbt_check(roadeside_parking_db.db_file_path, lane_identif_area_point_table, sizeof(lane_identif_area_point_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check lane_identif_area_point_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //车道区域识别区
        ret = dbt_check(roadeside_parking_db.db_file_path, lane_identif_area_table, sizeof(lane_identif_area_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check lane_identif_area_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //车道区域车道线
        ret = dbt_check(roadeside_parking_db.db_file_path, lane_line_table, sizeof(lane_line_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check lane_line_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //防抖动区点
        ret = dbt_check(roadeside_parking_db.db_file_path, detect_shake_point_table, sizeof(detect_shake_point_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check detect_shake_point_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //防抖动区
        ret = dbt_check(roadeside_parking_db.db_file_path, detect_shake_area_table, sizeof(detect_shake_area_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check detect_shake_area_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //融合区点
        ret = dbt_check(roadeside_parking_db.db_file_path, detect_fusion_point_table, sizeof(detect_fusion_point_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check detect_fusion_point_table Err,return -1", __FUNCTION__);
            return -1;
        }
        //融合区
        ret = dbt_check(roadeside_parking_db.db_file_path, detect_fusion_area_table, sizeof(detect_fusion_area_table)/sizeof(DBT_Table));
        if(ret != 0)
        {
            free(sqlstr);
            ERR("%s dbt_check detect_fusion_area_table Err,return -1", __FUNCTION__);
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
            "DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,PlateDefault,"
            "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
            "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
            "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
            "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress,CCK1Guid,CCK1IP,ErvPlatId) "
            "values(%d,'%s',%d,%d,%d,'%s',%d,'%s',%d,'%s','%s',"
            "'%s',%d,'%s',%d,'%s',%d,%d,'%s','%s',%d,'%s','%s','%s','%s')",
            data.Index, data.City.c_str(), data.IsUploadToPlatform, data.Is4Gmodel, 
            data.IsIllegalCapture, data.PlateDefault.c_str(), data.IsPrintIntersectionName, 
            data.FilesServicePath.c_str(), data.FilesServicePort, data.MainDNS.c_str(), 
            data.AlternateDNS.c_str(), data.PlatformTcpPath.c_str(), data.PlatformTcpPort, 
            data.PlatformHttpPath.c_str(), data.PlatformHttpPort, data.SignalMachinePath.c_str(), 
            data.SignalMachinePort, data.IsUseSignalMachine, data.NtpServerPath.c_str(),
            data.FusionMainboardIp.c_str(), data.FusionMainboardPort, data.IllegalPlatformAddress.c_str(), 
            data.CCK1Guid.c_str(), data.CCK1IP.c_str(), data.ErvPlatId.c_str());
    
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

    sprintf(sqlstr, "select id,DevIndex,City,IsUploadToPlatform,Is4Gmodel,IsIllegalCapture,PlateDefault,"
            "IsPrintIntersectionName,FilesServicePath,FilesServicePort,MainDNS,AlternateDNS,"
            "PlatformTcpPath,PlatformTcpPort,PlatformHttpPath,PlatformHttpPort,"
            "SignalMachinePath,SignalMachinePort,IsUseSignalMachine,NtpServerPath,"
            "FusionMainboardIp,FusionMainboardPort,IllegalPlatformAddress,CCK1Guid,CCK1IP,ErvPlatId "
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
        data.PlateDefault = sqldata[ncol+6]? sqldata[ncol+6]: "";
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
        data.CCK1Guid = sqldata[ncol+23]? sqldata[ncol+23]:"";
        data.CCK1IP = sqldata[ncol+24]? sqldata[ncol+24]:"";
        data.ErvPlatId = sqldata[ncol+25]? sqldata[ncol+25]:"";
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
int db_belong_intersection_insert(DB_Intersection_t &data)
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
int db_belong_intersection_get(DB_Intersection_t &data)
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

//雷达信息
int db_radar_info_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from radar_info");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_radar_info_insert(DB_Radar_Info_T &data)
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
    snprintf(sqlstr,1024,"insert into radar_info("
            "Guid,Name,Ip,Port,XThreshold,YThreshold,XCoordinate,YCoordinate,"
            "XRadarCoordinate,YRadarCoordinate,HRadarCoordinate,CorrectiveAngle,"
            "XOffset,YOffset,CheckDirection) "
            "values('%s','%s','%s',%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d)",
            data.Guid.c_str(), data.Name.c_str(), data.Ip.c_str(), data.Port, 
            data.XThreshold, data.YThreshold, data.XCoordinate, data.YCoordinate, 
            data.XRadarCoordinate, data.YRadarCoordinate, data.HRadarCoordinate, 
            data.CorrectiveAngle, data.XOffset, data.YOffset, data.CheckDirection);
    
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
int db_radar_info_get(DB_Radar_Info_T &data)
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

    sprintf(sqlstr, "select id,Guid,Name,Ip,Port,XThreshold,YThreshold,XCoordinate,"
            "YCoordinate,XRadarCoordinate,YRadarCoordinate,HRadarCoordinate,"
            "CorrectiveAngle,XOffset,YOffset,CheckDirection "
            "from radar_info order by id desc limit 1");
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
        data.Ip = sqldata[ncol+3]? sqldata[ncol+3]: "";
        data.Port = atoi(sqldata[ncol+4]?sqldata[ncol+4]: "0");
        data.XThreshold = atof(sqldata[ncol+5]? sqldata[ncol+5]: "0.0");
        data.YThreshold = atof(sqldata[ncol+6]? sqldata[ncol+6]: "0.0");
        data.XCoordinate = atof(sqldata[ncol+7]? sqldata[ncol+7]: "0.0");
        data.YCoordinate = atof(sqldata[ncol+8]? sqldata[ncol+8]: "0.0");
        data.XRadarCoordinate = atof(sqldata[ncol+9]? sqldata[ncol+9]: "0.0");
        data.YRadarCoordinate = atof(sqldata[ncol+10]? sqldata[ncol+10]: "0.0");
        data.HRadarCoordinate = atof(sqldata[ncol+11]? sqldata[ncol+11]: "0.0");
        data.CorrectiveAngle = atof(sqldata[ncol+12]? sqldata[ncol+12]: "0.0");
        data.XOffset = atof(sqldata[ncol+13]? sqldata[ncol+13]: "0.0");
        data.YOffset = atof(sqldata[ncol+14]? sqldata[ncol+14]: "0.0");
        data.CheckDirection = atoi(sqldata[ncol+15]? sqldata[ncol+15]: "0");
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

//相机信息
int db_camera_info_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from camera_info");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_camera_info_insert(DB_Camera_Info_T &data)
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
    snprintf(sqlstr,1024,"insert into camera_info("
            "Guid,Ip,Name,AccessMode,Type,CheckRange,Direction,PixelType,Delay,IsEnable) "
            "values('%s','%s','%s',%d,%d,%d,%d,%d,%d,%d)",
            data.Guid.c_str(), data.Ip.c_str(), data.Name.c_str(), data.AccessMode, data.Type, data.CheckRange, 
            data.Direction, data.PixelType, data.Delay, data.IsEnable);
    
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
int db_camera_info_get(vector <DB_Camera_Info_T> &data)
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

    sprintf(sqlstr, "select id,Guid,Ip,AccessMode,Type,CheckRange,Direction,PixelType,Delay,IsEnable,Name "
            "from camera_info where IsEnable=1");
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
    DB_Camera_Info_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.Guid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.Ip = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.AccessMode = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.Type = atoi(sqldata[index+4]? sqldata[index+4]: "0");
        tmp_data.CheckRange = atoi(sqldata[index+5]? sqldata[index+5]: "0");
        tmp_data.Direction = atoi(sqldata[index+6]? sqldata[index+6]: "0");
        tmp_data.PixelType = atoi(sqldata[index+7]? sqldata[index+7]: "0");
        tmp_data.Delay = atoi(sqldata[index+8]? sqldata[index+8]: "0");
        tmp_data.IsEnable = atoi(sqldata[index+9]? sqldata[index+9]: "0");
        tmp_data.Name = sqldata[index+10]? sqldata[index+10]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}

//相机详细配置
//存储相机详细配置，已存在相机-更新，不存在添加
int db_camera_detail_set_store(DB_Camera_Detail_Conf_T &data)
{
    int rtn = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;
    int index = 0;

    sqlstr = (char *)malloc(1024);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 256);
    sprintf(sqlstr, "select id from camera_detail_set where Guid='%s'", data.Guid.c_str());
    rtn = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.\n",sqlstr);
        free(sqlstr);
        return -1;
    }
    sqlite3_free_table_safe(sqldata);
    if(nrow == 0)
    {
        memset(sqlstr, 0x0, 1024);
        sprintf(sqlstr, "insert into camera_detail_set (Guid,DataVersion,Brightness,Contrast,Saturation,Acutance,MaximumGain,"
                    "ExposureTime,DigitalNoiseReduction,DigitalNoiseReductionLevel,WideDynamic,WideDynamicLevel,"
                    "BitStreamType,Resolution,Fps,CodecType,ImageQuality,BitRateUpper,"
                    "IsShowName,IsShowDate,IsShowWeek,TimeFormat,DateFormat,ChannelName,FontSize,"
                    "FontColor,Transparency,Flicker) "
                    "values ('%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
                    "%d,%d,%d,%d,'%s','%s',%d,'%s',%d,%d)", 
                    data.Guid.c_str(), data.DataVersion.c_str(), data.Brightness, data.Contrast, data.Saturation, 
                    data.Acutance, data.MaximumGain, data.ExposureTime, data.DigitalNoiseReduction, data.DigitalNoiseReductionLevel, 
                    data.WideDynamic, data.WideDynamicLevel, data.BitStreamType, data.Resolution, data.Fps, data.CodecType, 
                    data.ImageQuality, data.BitRateUpper, data.IsShowName, data.IsShowDate, data.IsShowWeek, 
                    data.TimeFormat, data.DateFormat.c_str(), data.ChannelName.c_str(), 
                    data.FontSize, data.FontColor.c_str(), data.Transparency, data.Flicker);
        rtn = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
        if (rtn < 0)
        {
            DBG("db_file_exec_sql:%s err.\n",sqlstr);
            free(sqlstr);
            return -1;
        }
    }
    else
    {
        memset(sqlstr, 0x0, 1024);
        sprintf(sqlstr, "update camera_detail_set set "
                    "Guid='%s',DataVersion='%s',Brightness=%d,Contrast=%d,Saturation=%d,Acutance=%d,MaximumGain=%d,"
                    "ExposureTime=%d,DigitalNoiseReduction=%d,DigitalNoiseReductionLevel=%d,WideDynamic=%d,WideDynamicLevel=%d,"
                    "BitStreamType=%d,Resolution=%d,Fps=%d,CodecType=%d,ImageQuality=%d,BitRateUpper=%d,"
                    "IsShowName=%d,IsShowDate=%d,IsShowWeek=%d,"
                    "TimeFormat=%d,DateFormat='%s',ChannelName='%s',FontSize=%d,FontColor='%s',Transparency=%d,Flicker=%d where Guid='%s'", 
                    data.Guid.c_str(), data.DataVersion.c_str(), data.Brightness, data.Contrast, data.Saturation, 
                    data.Acutance, data.MaximumGain, data.ExposureTime, data.DigitalNoiseReduction, data.DigitalNoiseReductionLevel, 
                    data.WideDynamic, data.WideDynamicLevel, data.BitStreamType, data.Resolution, data.Fps, data.CodecType, 
                    data.ImageQuality, data.BitRateUpper, data.IsShowName, data.IsShowDate, data.IsShowWeek, 
                    data.TimeFormat, data.DateFormat.c_str(), data.ChannelName.c_str(), 
                    data.FontSize, data.FontColor.c_str(), data.Transparency, data.Flicker, data.Guid.c_str());
        rtn = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
        if (rtn < 0)
        {
            DBG("db_file_exec_sql:%s err.\n",sqlstr);
            free(sqlstr);
            return -1;
        }
    }

    free(sqlstr);

    return 0;
}
//按相机guid取相机详细配置，能取到返回1，取不到返回0
int db_camera_detail_set_get_by_guid(char *guid, DB_Camera_Detail_Conf_T &data)
{
    int rtn = 0;
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
    memset(sqlstr, 0x0, 1024);
    sprintf(sqlstr, "select Guid,DataVersion,Brightness,Contras,Saturatio,Acutanc,MaximumGain,"
                    "ExposureTim,DigitalNoiseReduction,DigitalNoiseReductionLevel,WideDynami,WideDynamicLevel,"
                    "BitStreamTyp,Resolution,Fps,CodecType,ImageQuality,BitRateUpper,"
                    "IsShowName,IsShowDate,IsShowWeek,TimeForm,DateFormat,ChannelName,"
                    "FontSize,FontColor,Transparency,Flicker "
                    "from camera_detail_set order by id desc where Guid='%s'", guid);
    rtn = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.\n",sqlstr);
        free(sqlstr);
        return -1;
    }

    if(nrow > 0)
    {
        data.Guid = sqldata[ncol+0]? sqldata[ncol+0]: "";
        data.DataVersion = sqldata[ncol+1]? sqldata[ncol+1]: "";
        data.Brightness = atoi(sqldata[ncol+2]? sqldata[ncol+2]: "0");
        data.Contrast = atoi(sqldata[ncol+3]? sqldata[ncol+3]: "0");
        data.Saturation = atoi(sqldata[ncol+4]? sqldata[ncol+4]: "0");
        data.Acutance = atoi(sqldata[ncol+5]? sqldata[ncol+5]: "0");
        data.MaximumGain = atoi(sqldata[ncol+6]? sqldata[ncol+6]: "0");
        data.ExposureTime = atoi(sqldata[ncol+7]? sqldata[ncol+7]: "0");
        data.DigitalNoiseReduction = atoi(sqldata[ncol+8]? sqldata[ncol+8]: "0");
        data.DigitalNoiseReductionLevel = atoi(sqldata[ncol+9]? sqldata[ncol+9]: "0");
        data.WideDynamic = atoi(sqldata[ncol+10]? sqldata[ncol+10]: "0");
        data.WideDynamicLevel = atoi(sqldata[ncol+11]? sqldata[ncol+11]: "0");
        data.BitStreamType = atoi(sqldata[ncol+12]? sqldata[ncol+12]: "0");
        data.Resolution = atoi(sqldata[ncol+13]? sqldata[ncol+13]: "0");
        data.Fps = atoi(sqldata[ncol+14]? sqldata[ncol+14]: "0");
        data.CodecType = atoi(sqldata[ncol+15]? sqldata[ncol+15]: "0");
        data.ImageQuality = atoi(sqldata[ncol+16]? sqldata[ncol+16]: "0");
        data.BitRateUpper = atoi(sqldata[ncol+17]? sqldata[ncol+17]: "0");
        data.IsShowName = atoi(sqldata[ncol+18]? sqldata[ncol+18]: "0");
        data.IsShowDate = atoi(sqldata[ncol+19]? sqldata[ncol+19]: "0");
        data.IsShowWeek = atoi(sqldata[ncol+20]? sqldata[ncol+20]: "0");
        data.TimeFormat = atoi(sqldata[ncol+21]? sqldata[ncol+21]: "0");
        data.DateFormat = sqldata[ncol+22]? sqldata[ncol+22]: "";
        data.ChannelName = sqldata[ncol+23]? sqldata[ncol+23]: "";
        data.FontSize = atoi(sqldata[ncol+24]? sqldata[ncol+24]:"0");
        data.FontColor = sqldata[ncol+25]? sqldata[ncol+25]: "";
        data.Transparency = atoi(sqldata[ncol+26]? sqldata[ncol+26]: "0");
        data.Flicker = atoi(sqldata[ncol+27]? sqldata[ncol+27]: "0");

        rtn = 1;
    }
    else
    {
        rtn = 0;
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);

    return rtn;
}
//按相机guid取相机详细配置数据版本，能取到返回1，取不到返回0
int db_camera_detail_get_version(char *guid, char *version)
{
    int rtn = 0;
    char *sqlstr;
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sqlstr = (char *)malloc(256);
    if (sqlstr == NULL)
    {
        ERR("申请内存失败");
        return -1;
    }
    memset(sqlstr, 0x0, 256);
    sprintf(sqlstr, "select Guid,DataVersion from camera_detail_set where Guid='%s'", guid);
    rtn = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.\n",sqlstr);
        free(sqlstr);
        return -1;
    }

    if(nrow > 0)
    {
        sprintf(version, "%s", sqldata[ncol+1]? sqldata[ncol+1]: "");
        rtn = 1;
    }
    else
    {
        rtn = 0;
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);

    return rtn;
}

//车道信息
int db_lane_info_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from lane_info");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_lane_info_insert(DB_Lane_Info_T &data)
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
    snprintf(sqlstr,1024,"insert into lane_info("
            "Guid,Name,SerialNumber,DrivingDirection,Type,Length,Width,IsEnable,CameraName) "
            "values('%s','%s',%d,%d,%d,%f,%f,%d,'%s')",
            data.Guid.c_str(), data.Name.c_str(), data.SerialNumber, data.DrivingDirection, 
            data.Type, data.Length, data.Width, data.IsEnable, data.CameraName.c_str());
    
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
int db_lane_info_get(vector <DB_Lane_Info_T> &data)
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

    sprintf(sqlstr, "select id,Guid,Name,SerialNumber,DrivingDirection,Type,Length,Width,IsEnable,CameraName "
            "from lane_info");
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
    DB_Lane_Info_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.Guid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.Name = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.DrivingDirection = atoi(sqldata[index+4]? sqldata[index+4]: "0");
        tmp_data.Type = atoi(sqldata[index+5]? sqldata[index+5]: "0");
        tmp_data.Length = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        tmp_data.Width = atof(sqldata[index+7]? sqldata[index+7]: "0.0");
        tmp_data.IsEnable = atoi(sqldata[index+8]? sqldata[index+8]: "0");
        tmp_data.CameraName = sqldata[index+9]? sqldata[index+9]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//************识别相机
//车道区域识别区点
int db_lane_identif_point_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from lane_identif_area_point");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_lane_identif_point_insert(DB_Lane_Identif_Area_Point_T &data)
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
    snprintf(sqlstr,1024,"insert into lane_identif_area_point("
            "IdentifAreaGuid,SerialNumber,XPixel,YPixel) "
            "values('%s',%d,%f,%f)",
            data.IdentifAreaGuid.c_str(), data.SerialNumber, data.XPixel, data.YPixel);
    
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
int db_lane_identif_point_get(vector <DB_Lane_Identif_Area_Point_T> &data)
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
    sprintf(sqlstr, "select id,IdentifAreaGuid,SerialNumber,XPixel,YPixel "
            "from lane_identif_area_point");
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
    DB_Lane_Identif_Area_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.IdentifAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XPixel = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
int db_lane_identif_point_get_byguid(vector <DB_Lane_Identif_Area_Point_T> &data, const char* area_guid)
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
    sprintf(sqlstr, "select id,IdentifAreaGuid,SerialNumber,XPixel,YPixel "
            "from lane_identif_area_point where IdentifAreaGuid='%s'", area_guid);
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
    DB_Lane_Identif_Area_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.IdentifAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XPixel = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//车道区域识别区
int db_lane_identif_area_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from lane_identif_area");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_lane_identif_area_insert(DB_Lane_Identif_Area_T &data)
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
    snprintf(sqlstr,1024,"insert into lane_identif_area("
            "LaneGuid,IdentifAreaGuid,Type,CameraGuid) "
            "values('%s','%s',%d,'%s')",
            data.LaneGuid.c_str(), data.IdentifAreaGuid.c_str(), data.Type, data.CameraGuid.c_str());
    
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
int db_lane_identif_area_get_by_camera(vector <DB_Lane_Identif_Area_T> &data, const char* camera_guid)
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
    sprintf(sqlstr, "select id,LaneGuid,IdentifAreaGuid,Type,CameraGuid "
            "from lane_identif_area where CameraGuid='%s'", camera_guid);
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
    DB_Lane_Identif_Area_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.LaneGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.IdentifAreaGuid = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.Type = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.CameraGuid = sqldata[index+4]? sqldata[index+4]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
int db_lane_identif_area_get_by_guid(vector <DB_Lane_Identif_Area_T> &data, const char* lane_guid)
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
    sprintf(sqlstr, "select id,LaneGuid,IdentifAreaGuid,Type,CameraGuid "
            "from lane_identif_area where LaneGuid='%s'", lane_guid);
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
    DB_Lane_Identif_Area_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.LaneGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.IdentifAreaGuid = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.Type = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.CameraGuid = sqldata[index+4]? sqldata[index+4]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//车道区域车道线
int db_lane_line_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from lane_line");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_lane_line_insert(DB_Lane_Line_T &data)
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
    snprintf(sqlstr,1024,"insert into lane_line("
            "LaneGuid,LaneLineGuid,Type,Color,CoordinateSet,CameraGuid) "
            "values('%s','%s',%d,%d,'%s','%s')",
            data.LaneGuid.c_str(), data.LaneLineGuid.c_str(), data.Type, 
            data.Color, data.CoordinateSet.c_str(), data.CameraGuid.c_str());
    
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
int db_lane_line_get_by_camera(vector <DB_Lane_Line_T> &data, const char* camera_guid)
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
    sprintf(sqlstr, "select id,LaneGuid,LaneLineGuid,Type,Color,CoordinateSet,CameraGuid "
            "from lane_line where CameraGuid='%s'", camera_guid);
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
    DB_Lane_Line_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.LaneGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.LaneLineGuid = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.Type = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.Color = atoi(sqldata[index+4]? sqldata[index+4]: "0");
        tmp_data.CoordinateSet = sqldata[index+5]? sqldata[index+5]: "";
        tmp_data.CameraGuid = sqldata[index+6]? sqldata[index+6]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
int db_lane_line_get_by_guid(vector <DB_Lane_Line_T> &data, const char* lane_guid)
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
    sprintf(sqlstr, "select id,LaneGuid,LaneLineGuid,Type,Color,CoordinateSet,CameraGuid "
            "from lane_line where LaneGuid='%s'", lane_guid);
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
    DB_Lane_Line_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.LaneGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.LaneLineGuid = sqldata[index+2]? sqldata[index+2]: "";
        tmp_data.Type = atoi(sqldata[index+3]? sqldata[index+3]: "0");
        tmp_data.Color = atoi(sqldata[index+4]? sqldata[index+4]: "0");
        tmp_data.CoordinateSet = sqldata[index+5]? sqldata[index+5]: "";
        tmp_data.CameraGuid = sqldata[index+6]? sqldata[index+6]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//****************检测相机
//防抖动区点
int db_detect_shake_point_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from detect_shake_point");
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_detect_shake_point_insert(DB_Detect_Shake_Point_T &data)
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
    snprintf(sqlstr,1024,"insert into detect_shake_point("
            "ShakeAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel) "
            "values('%s',%d,%f,%f,%f,%f)",
            data.ShakeAreaGuid.c_str(), data.SerialNumber, data.XCoordinate, data.YCoordinate, data.XPixel, data.YPixel);
    
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
int db_detect_shake_point_get(vector <DB_Detect_Shake_Point_T> &data)
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
    sprintf(sqlstr, "select id,ShakeAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel "
            "from detect_shake_point");
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
    DB_Detect_Shake_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.ShakeAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XCoordinate = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YCoordinate = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        tmp_data.XPixel = atof(sqldata[index+5]? sqldata[index+5]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
int db_detect_shake_point_get_byguid(vector <DB_Detect_Shake_Point_T> &data, const char* shake_guid)
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
    sprintf(sqlstr, "select id,ShakeAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel "
            "from detect_shake_point where ShakeAreaGuid='%s'", shake_guid);
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
    DB_Detect_Shake_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.ShakeAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XCoordinate = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YCoordinate = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        tmp_data.XPixel = atof(sqldata[index+5]? sqldata[index+5]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//防抖动区
int db_detect_shake_area_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from detect_shake_area");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_detect_shake_area_insert(DB_Detect_Shake_Area_T &data)
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
    snprintf(sqlstr,1024,"insert into detect_shake_area("
            "Guid,XOffset,YOffset,CameraGuid) "
            "values('%s',%f,%f,'%s')",
            data.Guid.c_str(), data.XOffset, data.YOffset, data.CameraGuid.c_str());
    
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
int db_detect_shake_area_get(vector <DB_Detect_Shake_Area_T> &data, const char* camera_guid)
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

    sprintf(sqlstr, "select id,Guid,XOffset,YOffset,CameraGuid "
            "from detect_shake_area where CameraGuid='%s'", camera_guid);
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    data.clear();
    DB_Detect_Shake_Area_T tmp_data;
    for(int i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.Guid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.XOffset = atof(sqldata[index+2]? sqldata[index+2]: "0.0");
        tmp_data.YOffset = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.CameraGuid = sqldata[index+4]? sqldata[index+4]: "";
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//融合区点
int db_detect_fusion_p_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from detect_fusion_point");
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_detect_fusion_p_insert(DB_Detect_Fusion_Point_T &data)
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
    snprintf(sqlstr,1024,"insert into detect_fusion_point("
            "FusionAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel) "
            "values('%s',%d,%f,%f,%f,%f)",
            data.FusionAreaGuid.c_str(), data.SerialNumber, data.XCoordinate, data.YCoordinate, data.XPixel, data.YPixel);
    
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
int db_detect_fusion_p_get(vector <DB_Detect_Fusion_Point_T> &data)
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
    sprintf(sqlstr, "select id,FusionAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel "
            "from detect_fusion_point");
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
    DB_Detect_Fusion_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.FusionAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XCoordinate = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YCoordinate = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        tmp_data.XPixel = atof(sqldata[index+5]? sqldata[index+5]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
int db_detect_fusion_p_get_by_guid(vector <DB_Detect_Fusion_Point_T> &data, const char* fusion_guid)
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
    sprintf(sqlstr, "select id,FusionAreaGuid,SerialNumber,XCoordinate,YCoordinate,XPixel,YPixel "
            "from detect_fusion_point where FusionAreaGuid='%s'", fusion_guid);
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
    DB_Detect_Fusion_Point_T tmp_data;
    for(i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        tmp_data.FusionAreaGuid = sqldata[index+1]? sqldata[index+1]: "";
        tmp_data.SerialNumber = atoi(sqldata[index+2]? sqldata[index+2]: "0");
        tmp_data.XCoordinate = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        tmp_data.YCoordinate = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        tmp_data.XPixel = atof(sqldata[index+5]? sqldata[index+5]: "0.0");
        tmp_data.YPixel = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        data.push_back(tmp_data);
    }

    sqlite3_free_table_safe(sqldata);
    free(sqlstr);
    return 0;
}
//融合区
int db_detect_fusion_area_delete()
{
    int ret = 0;
    char sqlstr[128] = {0};

    sprintf(sqlstr,"delete from detect_fusion_area");
    
    ret = db_file_exec_sql(roadeside_parking_db.db_file_path, sqlstr, NULL, NULL);
    if(ret < 0)
    {
        DBG("db_file_exec_sql err.sqlstr:%s", sqlstr);
        return -1;
    }

    return 0;
}
int db_detect_fusion_area_insert(DB_Detect_Fusion_Area_T &data)
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
    snprintf(sqlstr,1024,"insert into detect_fusion_area("
            "Guid,PerspectiveTransFa00,PerspectiveTransFa01,PerspectiveTransFa02,PerspectiveTransFa10,PerspectiveTransFa11,"
            "PerspectiveTransFa12,PerspectiveTransFa20,PerspectiveTransFa21,PerspectiveTransFa22,XDistance,YDistance,HDistance,"
            "MPPW,MPPH,CameraGuid) "
            "values('%s',%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,'%s')",
            data.Guid.c_str(), data.PerspectiveTransFa00, data.PerspectiveTransFa01, data.PerspectiveTransFa02, 
            data.PerspectiveTransFa10, data.PerspectiveTransFa11, data.PerspectiveTransFa12, data.PerspectiveTransFa20, 
            data.PerspectiveTransFa21, data.PerspectiveTransFa22, data.XDistance, data.YDistance, data.HDistance, 
            data.MPPW, data.MPPH, data.CameraGuid.c_str());
    
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
int db_detect_fusion_area_get(vector <DB_Detect_Fusion_Area_T> &data, const char* camera_guid)
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

    sprintf(sqlstr, "select id,Guid,PerspectiveTransFa00,PerspectiveTransFa01,PerspectiveTransFa02,PerspectiveTransFa10,"
            "PerspectiveTransFa11,PerspectiveTransFa12,PerspectiveTransFa20,PerspectiveTransFa21,PerspectiveTransFa22,"
            "XDistance,YDistance,HDistance,MPPW,MPPH,CameraGuid "
            "from detect_fusion_area where CameraGuid='%s'", camera_guid);
    ret = db_file_exec_sql_table(roadeside_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (ret < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        sqlite3_free_table_safe(sqldata);
        free(sqlstr);
        return -1;
    }
    for(int i=0; i<nrow; i++)
    {
        int index = ncol * (i+1);
        DB_Detect_Fusion_Area_T area_data;
        area_data.Guid = sqldata[index+1]? sqldata[index+1]: "";
        area_data.PerspectiveTransFa00 = atof(sqldata[index+2]? sqldata[index+2]: "0.0");
        area_data.PerspectiveTransFa01 = atof(sqldata[index+3]? sqldata[index+3]: "0.0");
        area_data.PerspectiveTransFa02 = atof(sqldata[index+4]? sqldata[index+4]: "0.0");
        area_data.PerspectiveTransFa10 = atof(sqldata[index+5]? sqldata[index+5]: "0.0");
        area_data.PerspectiveTransFa11 = atof(sqldata[index+6]? sqldata[index+6]: "0.0");
        area_data.PerspectiveTransFa12 = atof(sqldata[index+7]? sqldata[index+7]: "0.0");
        area_data.PerspectiveTransFa20 = atof(sqldata[index+8]? sqldata[index+8]: "0.0");
        area_data.PerspectiveTransFa21 = atof(sqldata[index+9]? sqldata[index+9]: "0.0");
        area_data.PerspectiveTransFa22 = atof(sqldata[index+10]? sqldata[index+10]: "0.0");
        area_data.XDistance = atof(sqldata[index+11]? sqldata[index+11]: "0.0");
        area_data.YDistance = atof(sqldata[index+12]? sqldata[index+12]: "0.0");
        area_data.HDistance = atof(sqldata[index+13]? sqldata[index+13]: "0.0");
        area_data.MPPW = atof(sqldata[index+14]? sqldata[index+14]: "0.0");
        area_data.MPPH = atof(sqldata[index+15]? sqldata[index+15]: "0.0");
        area_data.CameraGuid = sqldata[index+16]? sqldata[index+16]: "";
        data.push_back(area_data);
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
//    uname = std::string("10600000-2541-4d75-b381-961fdd1da031");  //ceshi
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

/***取id最大一条数据的ParkingAreaID***/
int db_factory_get_ParkingAreaID(string &parkareaid)
{
    int rtn = 0;
    char sqlstr[256] = {0};
    char **sqldata;
    int nrow = 0;
    int ncol = 0;

    sprintf(sqlstr, "select ParkingAreaID from CL_ParkingArea where ID=(select max(ID) from CL_ParkingArea)");
    rtn = db_file_exec_sql_table(cl_parking_db.db_file_path, sqlstr, &sqldata, &nrow, &ncol);
    if (rtn < 0)
    {
        DBG("db_file_exec_sql_table:%s err.",sqlstr);
        return -1;
    }
    if(nrow == 1)
    {
        parkareaid = sqldata[1]? sqldata[1]: "";
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


