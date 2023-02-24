//
// Created by lining on 2023/2/23.
//

#ifndef DBTABLE_H
#define DBTABLE_H

#include <string>
#include <vector>

typedef struct {
    pthread_mutex_t db_mutex;
    std::string path;
    std::string version;
} DatabaseInfo;

#define HOME_PATH "/home/nvidianx"

extern DatabaseInfo eoc_configure;
extern DatabaseInfo CLParking;
extern DatabaseInfo RoadsideParking;

typedef struct {
    int id;
    std::string version;
    std::string time;
} DBDataVersion;

typedef struct {
    std::string tableName;            //数据库表名
    std::string columnName;            //列名
    std::string columnDescription;    //列描述
} DBTableInfo;

/****表字段检查配置******/
typedef struct {
    std::string name;
    std::string type;
} DBTableColInfo;

///////////具体的表内业务////////////

//核心板基础配置
typedef struct {
    int id;
    int Index;      //设备序号
    std::string City;    //城市
    int IsUploadToPlatform; //是否上传到云平台
    int Is4Gmodel;  //是否为4G模式
    int IsIllegalCapture;   //是否开启违法抓拍
    int IsPrintIntersectionName;//是否打印路口名称
    std::string FilesServicePath;//文件服务器地址
    int FilesServicePort;   //文件服务器端口
    std::string MainDNS;         //DNS主服务器
    std::string AlternateDNS;    //DNS备用服务器
    std::string PlatformTcpPath; //云平台TCP地址
    int PlatformTcpPort;    //云平台TCP端口
    std::string PlatformHttpPath;    //云平台HTTP地址
    int PlatformHttpPort;   //云平台HTTP端口
    std::string SignalMachinePath;   //信号机地址
    int SignalMachinePort;  //信号机端口
    int IsUseSignalMachine; //是否启用信号机接口
    std::string NtpServerPath;   //NTP服务器地址
    std::string FusionMainboardIp;   //主控机Ip
    int FusionMainboardPort;    //主控机端口号
    std::string IllegalPlatformAddress;  //违停服务器地址
    std::string Remarks;     //备注
} DBBaseSet;

//所属路口信息
typedef struct {
    int id;
    std::string Guid;    //guid
    std::string Name;    //路口名称
    int Type;   //路口的类型：1=十字形，2=X形，3=T形，4=Y形
    std::string PlatId;  //平台编号
    float XLength;  //x宽度(米)
    float YLength;  //y宽度(米)
    int LaneNumber; //车道数量
    std::string Latitude;    //路口中心点（原点）GPS坐标纬度
    std::string Longitude;   //路口中心点（原点）GPS坐标经度
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
} DBIntersection;
//融合参数设置
typedef struct {
    int id;
    double IntersectionAreaPoint1X; //路口中心区域标点1-X
    double IntersectionAreaPoint1Y; //路口中心区域标点1-Y
    double IntersectionAreaPoint2X; //路口中心区域标点2-X
    double IntersectionAreaPoint2Y; //路口中心区域标点2-Y
    double IntersectionAreaPoint3X; //路口中心区域标点3-X
    double IntersectionAreaPoint3Y; //路口中心区域标点3-Y
    double IntersectionAreaPoint4X; //路口中心区域标点4-X
    double IntersectionAreaPoint4Y; //路口中心区域标点4-Y
} DBFusionParaSet;
//关联设备
typedef struct {
    int id;
    int EquipType;  //设备类型
    std::string EquipCode;   //设备编码
} DBAssociatedEquip;
/**
 * 根据路径创建各级目录，最大路径长度512-1，递归创建知道最后一个'/'
 * @param path 将要递归创建的文件路径或文件夹路径
 * @return 0：成功 -1：失败
 */
int mkdirFromPath(const char* path);

/**
* eoc配置表初始化，如果表不存在则创建表
* @return 0 成功  -1 失败
*/
int tableInit(std::string path, std::string version);
/**
 * 校验一个数据库表，如果可能会根据描述创建一个表
 * @param db_path
 * @param table
 * @param column_size
 * @return 0 成功  -1 失败
 */
int checkTable(const char* db_path, const DBTableInfo *table, int column_size);

/**
 * 检查表内字段，不存在的话则添加
 * @param tab_name
 * @param tab_column
 * @param check_num
 * @return
 */
int checkTableColumn(std::string tab_name, DBTableColInfo *tab_column, int check_num);


int deleteVersion();

int insertVersion(DBDataVersion &data);

int getVersion(std::string &version);

//基础配置信息
int delete_base_set();

int insert_base_set(DBBaseSet data);

int get_base_set(DBBaseSet &data);

//所属路口信息
int delete_belong_intersection();

int insert_belong_intersection(DBIntersection data);

int get_belong_intersection(DBIntersection &data);

//融合参数设置
int delete_fusion_para_set();

int insert_fusion_para_set(DBFusionParaSet data);

int get_fusion_para_set(DBFusionParaSet &data);

//关联设备
int delete_associated_equip();

int insert_associated_equip(DBAssociatedEquip data);

int get_associated_equip(std::vector<DBAssociatedEquip> &data);

//其他操作
int dbGetCloudInfo(std::string &server_path, int &server_port, std::string &file_server_path,
                                           int &file_server_port);

/***取id最大一条数据的用户名***/
int dbGetUname(std::string &uname);


#endif //DBTABLE_H
