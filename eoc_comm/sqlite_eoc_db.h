
#ifndef _SQLITE_EOC_DB_H_
#define _SQLITE_EOC_DB_H_

#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <vector>

using namespace std;

//#define SQL_BUFFERSIZE 2048

#define NX 1
#define HI3516 0
#define HWJ 0

#if NX
#define HOME_PATH "/home/nvidianx"
//#define HOME_PATH "/home/nvidia"
#elif HI3516
#define HOME_PATH "/home/zhht/run"
#endif


typedef struct DB_Conf_Data_Version_t
{
    int Id;
    string version;
    string time;
}DB_Conf_Data_Version;


//核心板基础配置
typedef struct
{
    int id;
    int Index;      //设备序号
    string City;    //城市
    int IsUploadToPlatform; //是否上传到云平台
    int Is4Gmodel;  //是否为4G模式
    int IsIllegalCapture;   //是否开启违法抓拍
    int IsPrintIntersectionName;//是否打印路口名称
    string FilesServicePath;//文件服务器地址
    int FilesServicePort;   //文件服务器端口
    string MainDNS;         //DNS主服务器
    string AlternateDNS;    //DNS备用服务器
    string PlatformTcpPath; //云平台TCP地址
    int PlatformTcpPort;    //云平台TCP端口
    string PlatformHttpPath;    //云平台HTTP地址
    int PlatformHttpPort;   //云平台HTTP端口
    string SignalMachinePath;   //信号机地址
    int SignalMachinePort;  //信号机端口
    int IsUseSignalMachine; //是否启用信号机接口
    string NtpServerPath;   //NTP服务器地址
    string FusionMainboardIp;   //主控机Ip
    int FusionMainboardPort;    //主控机端口号
    string IllegalPlatformAddress;  //违停服务器地址
    string Remarks;     //备注
}DB_Base_Set_T;
//所属路口信息
typedef struct
{
    int id;
    string Guid;    //guid
    string Name;    //路口名称
    int Type;   //路口的类型：1=十字形，2=X形，3=T形，4=Y形
    string PlatId;  //平台编号
    float XLength;  //x宽度(米)
    float YLength;  //y宽度(米)
    int LaneNumber; //车道数量
    string Latitude;    //路口中心点（原点）GPS坐标纬度
    string Longitude;   //路口中心点（原点）GPS坐标经度
    //-----路口参数设置
    int FlagEast;   // 十字路口设定标志位 是否启用偏移方向-东
    int FlagSouth;  // 十字路口设定标志位 是否启用偏移方向-南
    int FlagWest;   // 十字路口设定标志位  是否启用偏移方向-西
    int FlagNorth;  // 十字路口设定标志位  是否启用偏移方向-北
    double DeltaXEast;  // 偏移量X-东
    double DeltaYEast;  // 偏移量Y-东
    double DeltaXSouth; // 偏移量X-南
    double DeltaYSouth; // 偏移量Y-南
    double DeltaXWest;  // 偏移量X-西
    double DeltaYWest;  // 偏移量Y-西
    double DeltaXNorth; // 偏移量X-北
    double DeltaYNorth; // 偏移量Y-北
    double WidthX;      // 融合前去重重合车辆用偏移量X
    double WidthY;      // 融合前去重重合车辆用偏移量Y
}DB_Intersection_T;
//融合参数设置
typedef struct
{
    int id;
    double IntersectionAreaPoint1X; //路口中心区域标点1-X
    double IntersectionAreaPoint1Y; //路口中心区域标点1-Y
    double IntersectionAreaPoint2X; //路口中心区域标点2-X
    double IntersectionAreaPoint2Y; //路口中心区域标点2-Y
    double IntersectionAreaPoint3X; //路口中心区域标点3-X
    double IntersectionAreaPoint3Y; //路口中心区域标点3-Y
    double IntersectionAreaPoint4X; //路口中心区域标点4-X
    double IntersectionAreaPoint4Y; //路口中心区域标点4-Y
}DB_Fusion_Para_Set_T;
//关联设备
typedef struct
{
    int id;
    int EquipType;  //设备类型
    string EquipCode;   //设备编码
}DB_Associated_Equip_T;


/*
 * eoc配置表初始化，如果表不存在则创建表
 *
 * 0：成功
 * -1：失败
 * */
int eoc_db_table_init();
int db_version_delete();
int db_version_insert(DB_Conf_Data_Version &data);
int db_version_get(string &conf_version);

/////////////////业务相关配置表
//基础配置信息
int db_base_set_delete();
int db_base_set_insert(DB_Base_Set_T &data);
int db_base_set_get(DB_Base_Set_T &data);

//所属路口信息
int db_belong_intersection_delete();
int db_belong_intersection_insert(DB_Intersection_T &data);
int db_belong_intersection_get(DB_Intersection_T &data);

//融合参数设置
int db_fusion_para_set_delete();
int db_fusion_para_set_insert(DB_Fusion_Para_Set_T &data);
int db_fusion_para_set_get(DB_Fusion_Para_Set_T &data);

//关联设备
int db_associated_equip_delete();
int db_associated_equip_insert(DB_Associated_Equip_T &data);
int db_associated_equip_get(vector <DB_Associated_Equip_T> &data);


//更新~/bin/RoadsideParking.db下eoc服务器地址和文件服务器地址
int bin_parking_lot_update_eocpath(const char *serverPath, int serverPort, const char *filePath, int filePort);
int db_parking_lot_get_cloud_addr_from_factory(string &server_path, int* server_port, string &file_server_path, int* file_server_port);
/***取id最大一条数据的用户名***/
int db_factory_get_uname(string &uname);



#endif

