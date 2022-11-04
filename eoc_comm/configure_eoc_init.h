
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
    char PlateDefault[32];    //无牌车默认字段
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
    char CCK1Guid[64];    //矩阵控制器K1的guid
    char CCK1Ip[32];      //矩阵控制器K1的ip
    char ErvPlatId[64];   //RV串号
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
//相机信息
typedef struct
{
    char Guid[64];    //guid
    char Ip[32];  //IP
    char Name[32];  //Name
    int AccessMode; //接入方式:1=usb接入，2=网络接入
    int Type;   //相机类型：1=检测相机，2=识别相机
    int CheckRange; //检测范围：1=远端，2=近端
    int Direction;  //相机朝向：1=东，2=南，3=西，4=北
    int PixelType;  //相机像素：1=200万，2=400万，3=800万
    int Delay;      //相机延时
    int IsEnable;   //是否启用：1=启用，2=不启用
}EOC_Camera_Info;
//雷达信息
typedef struct
{
    char Guid[64];    //guid
    char Name[64];  //雷达Name
    char IP[64];    //雷达地址
    int Port;       //雷达端口号
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
}EOC_Radar_Info;
//相机详细配置
typedef struct {
    char Guid[64];    //相机guid
    char DataVersion[64]; //相机数据版本号
    
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
    char DateFormat[64];  //日期格式
    char ChannelName[64]; //通道名称
    int FontSize;       //字体大小 1：16，2：32，3：48，4：64
    char FontColor[64];   // 字体颜色
    int Transparency;   // 透明度 0=不透明，1=透明
    int Flicker;        // 闪烁  0=不闪烁，1=闪烁
}EOC_Camera_Detail_info;

//车道信息
//-------车道线
typedef struct
{
    string LaneLineGuid;
    int Type;       // 车道线类型：1=虚线，2=实线，3=双实线，4=双虚线，5=上虚下实线，6=下虚上实线
    int Color;      // 车道线颜色：1=白色，2=黄色
    string CoordinateSet;   //车道线点坐标集合
}EOC_Lane_Line;
/////////////////////////////////////
//-------识别区
typedef struct
{
    int SerialNumber;   //点序号
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}EOC_Identif_Area_Point;
typedef struct
{
    string Guid;
    int Type; //1=车牌识别区，2=车头识别区，3=车尾识别区
    std::vector<EOC_Identif_Area_Point> IdentificationAreaPoints;
}EOC_Identif_Area;
//////////////////////////////////////
//车道区域
typedef struct
{
    string LaneGuid;
    std::vector<EOC_Lane_Line> LaneLineInfos;//车道线
    std::vector<EOC_Identif_Area> IdentificationAreas;//识别区
}EOC_Lane_Area;
//车道
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

//    EOC_Lane_Area LaneAreaInfos;    //删
}EOC_Lane_Info;


//检测相机下的——防抖动区
typedef struct
{
    int SerialNumber;   //点序号
    float XCoordinate;  //相对路口x坐标
    float YCoordinate;  //相对路口y坐标
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}EOC_Shake_Point;
typedef struct
{
    string Guid;
    float XOffset;// 抖动位偏移x
    float YOffset;// 抖动位偏移Y
    std::vector<EOC_Shake_Point> ShakePoints;
}EOC_Shake_Area;          //防抖动区
//检测相机下的——融合区
typedef struct
{
    int SerialNumber;   //点序号
    float XCoordinate;  //相对路口x坐标
    float YCoordinate;  //相对路口y坐标
    float XPixel;       //像素横坐标x值
    float YPixel;       //像素横坐标x值
}EOC_Fusion_Point;
typedef struct
{
    string Guid;
    std::vector<EOC_Fusion_Point> FusionPoints;
    double PerspectiveTransFa[3][3]; // 系数矩阵-透视变换用系数矩阵，3*3矩阵
    double XDistance;  //变化实测距离X-透视变换最左边的直线与Y轴之间的实际测量距离
    double YDistance; //变化实测距离Y-透视变换最下面的直线与X轴之间的世界测量距离
    double HDistance; // 变换实测距离H-透视变化图像高度
    double MPPW; //宽度米像素
    double MPPH; //高度米像素
}EOC_Fusion_Area;          //融合区

//相机所辖区域信息
typedef struct
{
    string CameraGuid;
    std::vector<EOC_Shake_Area> ShakeAreas; //防抖区
    std::vector<EOC_Fusion_Area> FusionAreas;   //融合区
    std::vector<EOC_Lane_Area> LaneAreas;   //车道区域
}EOC_Camera_Area_Info;


//返回值：1，取到eoc配置；0，eoc配置没下发；-1，初始化数据库失败
int g_eoc_config_init(void);

//通过相机guid获取相机详细配置
int get_camera_detail_set(const char* camera_guid, EOC_Camera_Detail_info &data);
//取相机类型
//返回值：1=检测相机，2=识别相机，0没查到相机
int get_camera_type(const char* camera_ip);
//取相机检测范围
//返回值：1=远端，2=近端，0没查到相机
int get_camera_check_range(const char* camera_ip);
//根据相机guid取相机ip
//返回值：相机ip
string get_camera_ip(const char* camera_guid);

//根据矩阵控制器guid取ip
//返回值：0=取到ip，-1=输入guid错误
int get_controller_ip(string &guid, string &ip);

int get_camera_shake_area_info(const char* camera_guid, vector <EOC_Shake_Area> &data);
int get_camera_fusion_area_info(const char* camera_guid, vector <EOC_Fusion_Area> &data);
int get_camera_lane_area_info(const char* camera_guid, vector <EOC_Lane_Area> &data);

//根据相机ip取相机所有区域信息，包括防抖区，融合区，车道区
//返回值：-1错误，未找到相机对应区域配置信息   ；0正确
int get_camera_area_set_info(const char* ip, EOC_Camera_Area_Info &data);


#endif

