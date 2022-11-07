
#ifndef _CONFIGURE_PARK_INIT_H_
#define _CONFIGURE_PARK_INIT_H_

#include <vector>
#include <string>

using namespace std;

//核心板基础配置
typedef struct
{
    int Index;      //设备序号
    char City[64];    //城市
    int IsUploadToPlatform; //是否上传到云平台
    int Is4Gmodel;  //是否为4G模式
    int IsIllegalCapture;   //是否开启违法抓拍
    char Remarks[64];    //无牌车默认字段
    int IsPrintIntersectionName;//是否打印路口名称
    char FilesServicePath[128];//文件服务器地址
    int FilesServicePort;   //文件服务器端口
    char MainDNS[32];         //DNS主服务器
    char AlternateDNS[32];    //DNS备用服务器
    char PlatformTcpPath[128]; //云平台TCP地址
    int PlatformTcpPort;    //云平台TCP端口
    char PlatformHttpPath[128];    //云平台HTTP地址
    int PlatformHttpPort;   //云平台HTTP端口
    char SignalMachinePath[128];   //信号机地址
    int SignalMachinePort;  //信号机端口
    int IsUseSignalMachine; //是否启用信号机接口
    char NtpServerPath[128];   //NTP服务器地址
    char FusionMainboardIp[32];   //前端融合主控机Ip
    int FusionMainboardPort;    //前端融合主控机端口号
    char IllegalPlatformAddress[32];  //违停服务器地址
    char EocConfDataVersion[64];    //eoc配置数据版本号
}EOC_Base_Set;
//所属路口信息
typedef struct
{
    char Guid[64];    //guid
    char Name[128];    //路口名称
    int Type;   //路口的类型：1=十字形，2=X形，3=T形，4=Y形
    char PlatId[64];  //平台编号
    float XLength;  //x宽度(米)
    float YLength;  //y宽度(米)
    int LaneNumber; //车道数量
    char Latitude[64];    //路口中心点（原点）GPS坐标纬度
    char Longitude[64];   //路口中心点（原点）GPS坐标经度
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
}EOC_Intersection;
//融合参数设置
typedef struct
{
    double IntersectionAreaPoint1X; //路口中心区域标点1-X
    double IntersectionAreaPoint1Y; //路口中心区域标点1-Y
    double IntersectionAreaPoint2X; //路口中心区域标点2-X
    double IntersectionAreaPoint2Y; //路口中心区域标点2-Y
    double IntersectionAreaPoint3X; //路口中心区域标点3-X
    double IntersectionAreaPoint3Y; //路口中心区域标点3-Y
    double IntersectionAreaPoint4X; //路口中心区域标点4-X
    double IntersectionAreaPoint4Y; //路口中心区域标点4-Y
}EOC_Fusion_Para_Set;
//关联设备
typedef struct
{
    int EquipType;  //设备类型
    char EquipCode[128];   //设备编码
}EOC_Associated_Equip;



//返回值：1，取到eoc配置；0，eoc配置没下发；-1，初始化数据库失败
int g_eoc_config_init(void);
//从~/bin/RoadsideParking.db取eoc服务器地址
int get_eoc_server_addr(string &server_path, int* server_port, string &file_server_path, int* file_server_port);

#endif

