
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
//    string DataVersion; //数据版本号
    int Index;      //设备序号
    string City;    //城市
    int IsUploadToPlatform; //是否上传到云平台
    int Is4Gmodel;  //是否为4G模式
    int IsIllegalCapture;   //是否开启违法抓拍
    string PlateDefault;    //无牌车默认字段
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
    string FusionMainboardIp;   //前端融合主控机Ip
    int FusionMainboardPort;    //前端融合主控机端口号
    string IllegalPlatformAddress;  //违停服务器地址
    string CCK1Guid;    //矩阵控制器K1的guid
    string CCK1IP;      //矩阵控制器K1的ip
    string ErvPlatId;   //RV串号
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
}DB_Intersection_t;
//相机信息
typedef struct
{
    int id;
    string Guid;    //guid
    string Ip;  //IP
    string Name;    //相机名称
    int AccessMode; //接入方式:1=usb接入，2=网络接入
    int Type;   //相机类型：1=检测相机，2=识别相机
    int CheckRange; //检测范围：1=远端，2=近端
    int Direction;  //相机朝向：1=东，2=南，3=西，4=北
    int PixelType;  //相机像素：1=200万，2=400万，3=800万
    int Delay;      //相机延时
    int IsEnable;   //是否启用：1=启用，2=不启用
}DB_Camera_Info_T;
//雷达信息
typedef struct
{
    int id;
    string Guid;    //guid
    string Name;    //雷达name
    string Ip;      //ip地址
    int Port;       //端口号
    float XThreshold;   //x方向门限值
    float YThreshold;   //y 方向门限值
    float XCoordinate;  //相对路口-坐标系x
    float YCoordinate;  //相对路口-坐标系y
    float XRadarCoordinate; //雷视坐标系x
    float YRadarCoordinate; //雷视坐标系y
    float HRadarCoordinate; //雷视坐标系h
    float CorrectiveAngle;  //雷达矫正角度
    float XOffset;  //雷达偏移量x
    float YOffset;  //雷达偏移量Y
    int CheckDirection; //检测方向:1=北向南，2=南向北，3=东向西，4=西向东
}DB_Radar_Info_T;
//相机详细配置
typedef struct {
    string Guid;    //相机guid
    string DataVersion; //相机数据版本号
    
    /*ImageAdjustmentParam 图像调节*/
    int Brightness; //亮度
    int Contrast;   //对比度
    int Saturation; //饱和度
    int Acutance;   //锐度
    int MaximumGain;    //最大增益
    int ExposureTime;   //曝光时间，枚举值1:1/25,2:1/50,3:1/75,4:1/100,5:1/120,
                                //6:1/150,7:1/175,8:1/200,9:1/225,10:1/250,
                                //11:1/300,12:1/425,13:1/600,14:1/1000,15:1/1250,
                                //16:1/1750,17:1/2500,18:1/3500,19:1/6000
    
    /*ImageEnhanceParam 图像增强*/
    int DigitalNoiseReduction;      //数字降噪 枚举值1：关闭，2：普通模式
    int DigitalNoiseReductionLevel; //降噪等级
    
    /*BackLightAdjustmentParam 背光调节*/
    int WideDynamic;        //宽动态1：关闭，2：开启
    int WideDynamicLevel;   //宽动态等级
    
    /*VideoCodingParam 视频编码*/
    int BitStreamType;  //码流类型 1 主码流, 2 子码流
    int Resolution;     //分辨率 1 3840X2160, 2 3072X2048, 3 3072X1728, 4 2592X1944,
                        //5 2688X1520, 6 2048X1536, 7 2304X1296, 8 1920X1080, 9 1280X960, 10 1280X720
    int Fps;            //视频帧率 1：1/16，2：1/8，3：1/4，4：1/2，5：1，6：2，7：4，8：6，
                            //9：8，10：10，11：12，12：15，13：16，14：18，15：20，16：22，17：25，18：10
    int CodecType;      //编码格式 1 H264, 2 H265
    int ImageQuality;   //图像质量 1：低，2：中，3：高
    int BitRateUpper;   //码率上限 1：256，2：512，3：1024，4：2048，5：3072，6：4096，7：6144，
                            //8：8192，9：12288，10：16384
    
    /*VideoOsdParam 视频OSD*/
    int IsShowName;     //是否显示名称
    int IsShowDate;     //是否显示日期
    int IsShowWeek;     //是否显示星期

    int TimeFormat;     //时间格式 1=24小时制 2=12小时制
    string DateFormat;  //日期格式
    string ChannelName; //通道名称
    int FontSize;       //字体大小 1：16，2：32，3：48，4：64
    string FontColor;   // 字体颜色
    int Transparency;   // 透明度 0=不透明，1=透明
    int Flicker;        // 闪烁  0=不闪烁，1=闪烁
}DB_Camera_Detail_Conf_T;
//车道信息
typedef struct
{
    string Guid;    //guid
    string Name;    //车道名称
    int SerialNumber;       // 车道号
    int DrivingDirection;   //行驶方向：1=东，2=南，3=西，4=北
    int Type;               // 车道类型：1=直行车道，2=左转车道，3=右转车道，4=掉头车道，5=直行-左转车道，
                                //6=直行右转车道，7=直行-掉头车道，8=左转-掉头车道，9=直行-左转-掉头车道
    float Length;   //长度
    float Width;    //宽度
    int IsEnable;   //是否启用 1=启用，2=不启用
    string CameraName;  // 识别相机名称
}DB_Lane_Info_T;
//************识别相机++++检测相机
//车道区域识别区点
typedef struct
{
    string IdentifAreaGuid; //所属识别区guid
    int SerialNumber;   //点序号
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}DB_Lane_Identif_Area_Point_T;
//车道区域识别区
typedef struct
{
    string LaneGuid; //所属车道guid
    string IdentifAreaGuid; //识别区guid
    int Type; //1=车牌识别区，2=车头识别区，3=车尾识别区
    string CameraGuid;  //所属相机guid
}DB_Lane_Identif_Area_T;
//车道区域车道线
typedef struct
{
    string LaneGuid; //所属车道guid
    string  LaneLineGuid;
    int Type;       // 车道线类型：1=虚线，2=实线，3=双实线，4=双虚线，5=上虚下实线，6=下虚上实线
    int Color;      // 车道线颜色：1=白色，2=黄色
    string CoordinateSet;   //车道线点坐标集合
    string CameraGuid;  //所属相机guid
}DB_Lane_Line_T;
//****************检测相机
//防抖动区
typedef struct
{
    string ShakeAreaGuid;   //所属防抖区guid
    int SerialNumber;   //点序号
    float XCoordinate;  //相对路口x坐标
    float YCoordinate;  //相对路口y坐标
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}DB_Detect_Shake_Point_T;
typedef struct
{
    string Guid;
    float XOffset;// 抖动位偏移x
    float YOffset;// 抖动位偏移Y
    string CameraGuid;  //所属相机guid
}DB_Detect_Shake_Area_T;          //防抖动区
//融合区
typedef struct
{
    string FusionAreaGuid;  //所属融合区guid
    int SerialNumber;   //点序号
    float XCoordinate;  //相对路口x坐标
    float YCoordinate;  //相对路口y坐标
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}DB_Detect_Fusion_Point_T;
typedef struct
{
    string Guid;
//    double PerspectiveTransFa[3][3]; // 系数矩阵-透视变换用系数矩阵，3*3矩阵
    double PerspectiveTransFa00;
    double PerspectiveTransFa01;
    double PerspectiveTransFa02;
    double PerspectiveTransFa10;
    double PerspectiveTransFa11;
    double PerspectiveTransFa12;
    double PerspectiveTransFa20;
    double PerspectiveTransFa21;
    double PerspectiveTransFa22;
    double XDistance;  //变化实测距离X-透视变换最左边的直线与Y轴之间的实际测量距离
    double YDistance; //变化实测距离Y-透视变换最下面的直线与X轴之间的世界测量距离
    double HDistance; // 变换实测距离H-透视变化图像高度
    double MPPW; //宽度米像素
    double MPPH; //高度米像素
    string CameraGuid;  //所属相机guid
}DB_Detect_Fusion_Area_T;          //融合区




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
int db_belong_intersection_insert(DB_Intersection_t &data);
int db_belong_intersection_get(DB_Intersection_t &data);

//雷达信息
int db_radar_info_delete();
int db_radar_info_insert(DB_Radar_Info_T &data);
int db_radar_info_get(DB_Radar_Info_T &data);

//相机信息
int db_camera_info_delete();
int db_camera_info_insert(DB_Camera_Info_T &data);
int db_camera_info_get(vector <DB_Camera_Info_T> &data);

//相机详细设置
int db_camera_detail_set_store(DB_Camera_Detail_Conf_T &data);
int db_camera_detail_set_get_by_guid(char *guid, DB_Camera_Detail_Conf_T &data);
//按相机guid取相机详细配置数据版本，能取到返回1，取不到返回0
int db_camera_detail_get_version(char *guid, char *version);

//车道信息
int db_lane_info_delete();
int db_lane_info_insert(DB_Lane_Info_T &data);
int db_lane_info_get(vector <DB_Lane_Info_T> &data);

//************识别相机
//车道区域识别区点
int db_lane_identif_point_delete();
int db_lane_identif_point_insert(DB_Lane_Identif_Area_Point_T &data);
int db_lane_identif_point_get(vector <DB_Lane_Identif_Area_Point_T> &data);
int db_lane_identif_point_get_byguid(vector <DB_Lane_Identif_Area_Point_T> &data, const char* area_guid);

//车道区域识别区
int db_lane_identif_area_delete();
int db_lane_identif_area_insert(DB_Lane_Identif_Area_T &data);
int db_lane_identif_area_get_by_camera(vector <DB_Lane_Identif_Area_T> &data, const char* camera_guid);
int db_lane_identif_area_get_by_guid(vector <DB_Lane_Identif_Area_T> &data, const char* lane_guid);

//车道区域车道线
int db_lane_line_delete();
int db_lane_line_insert(DB_Lane_Line_T &data);
int db_lane_line_get_by_camera(vector <DB_Lane_Line_T> &data, const char* camera_guid);
int db_lane_line_get_by_guid(vector <DB_Lane_Line_T> &data, const char* lane_guid);

//****************检测相机
//防抖动区点
int db_detect_shake_point_delete();
int db_detect_shake_point_insert(DB_Detect_Shake_Point_T &data);
int db_detect_shake_point_get(vector <DB_Detect_Shake_Point_T> &data);
int db_detect_shake_point_get_byguid(vector <DB_Detect_Shake_Point_T> &data, const char* shake_guid);

//防抖动区
int db_detect_shake_area_delete();
int db_detect_shake_area_insert(DB_Detect_Shake_Area_T &data);
int db_detect_shake_area_get(vector <DB_Detect_Shake_Area_T> &data, const char* camera_guid);

//融合区点
int db_detect_fusion_p_delete();
int db_detect_fusion_p_insert(DB_Detect_Fusion_Point_T &data);
int db_detect_fusion_p_get(vector <DB_Detect_Fusion_Point_T> &data);
int db_detect_fusion_p_get_by_guid(vector <DB_Detect_Fusion_Point_T> &data, const char* fusion_guid);

//融合区
int db_detect_fusion_area_delete();
int db_detect_fusion_area_insert(DB_Detect_Fusion_Area_T &data);
int db_detect_fusion_area_get(vector <DB_Detect_Fusion_Area_T> &data, const char* camera_guid);



//更新~/bin/RoadsideParking.db下eoc服务器地址和文件服务器地址
int bin_parking_lot_update_eocpath(const char *serverPath, int serverPort, const char *filePath, int filePort);
int db_parking_lot_get_cloud_addr_from_factory(string &server_path, int* server_port, string &file_server_path, int* file_server_port);
/***取id最大一条数据的用户名***/
int db_factory_get_uname(string &uname);
/***取id最大一条数据的ParkingAreaID***/
int db_factory_get_ParkingAreaID(string &parkareaid);



#endif

