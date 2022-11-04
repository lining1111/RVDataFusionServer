#ifndef __YYLS_EOC_COMM_HPP__
#define __YYLS_EOC_COMM_HPP__

//g++ -o ls_eoc_test src/*.cpp src/*.c src/camera/*.cpp -Isrc/ -Isrc/camera/ -std=c++11 -Isrc/camera/ -g -lssl -lcrypto -lpthread -L/usr/local/lib -I/usr/local/include -lsqlite3 -lcurl -lXNetSDK `pkg-config --libs opencv`
//nx编译 g++ -o ls_eoc_test src/*.cpp src/*.c src/camera/*.cpp -Isrc/ -Isrc/camera/ -std=c++11 -g -lssl -lcrypto -lpthread -L/usr/local/lib -I/usr/local/include -lsqlite3 -lcurl -lXNetSDK `pkg-config --cflags opencv4` `pkg-config --libs opencv4`

#include <string>
#include <vector>
#include "ttcp.hpp"

using namespace std;


#define EOC_COMMUNICATION_VERSION "V1.0.0"  //eoc通信协议版本号


/**********EOC下载、升级***********/
typedef enum EocDownloadStatus
{
      DOWNLOAD_IDLE = 1,
      DOWNLOAD_RUNNING,     //暂未用
      DOWNLOAD_FINISHED,
      DOWNLOAD_FILE_NOT_EXIST,
      DOWNLOAD_TIMEOUT,
      DOWNLOAD_SPACE_NOT_ENOUGH,
      DOWNLOAD_MD5_FAILD,
      DOWNLOAD_WATI_UPGRADE
}EocDownloadStatus;
typedef enum EocUpgradeDevType
{
      EOC_UPGRADE_PARKINGAREA = 1,  //主控板升级
      EOC_UPGRADE_CAMARA,           //相机升级
      EOC_UPGRADE_CONTROLLER        //矩阵控制器升级
}EocUpgradeDevType;
typedef enum EocUpgradeStatus
{
      EOC_UPGRADE_IDLE = 1, //初始添加升级
      EOC_UPGRADE_RUNNING,  //升级中
      EOC_UPGRADE_SUCCESS,  //升级成功
      EOC_UPGRADE_FAILED    //升级失败
}EocUpgradeStatus;

typedef struct EocUpgradeDev_t{
    string dev_guid;
    string dev_ip;
    EocUpgradeDevType dev_type;
    time_t start_upgrade_time;
    string sw_version;
    string hw_version;
    string dev_model;   //设备型号——暂只用于相机
    int upgrade_mode;   //升级方式1选择升级；2强制升级
    string upgrade_time;
    string comm_guid;   //通信guid
    EocUpgradeStatus up_status; //升级状态
    int up_progress;    //升级进度 0~100：状态为开始升级->进度进到10，升级中->10~99，升级完成(成功/失败)->100
}EocUpgradeDev;

typedef struct EocDownloadsMsg_t{
    vector<EocUpgradeDev> upgrade_dev;
    EocDownloadStatus download_status;
    pthread_t download_thread_id;
    string download_url;
    string download_file_name;
    int download_file_size;
    string download_file_md5;
//    string comm_guid;
}EocDownloadsMsg;

typedef enum EOCCLOUD_TASK{
    SYS_REBOOT = 0,     //重启进程
    GET_PK_RULE,        //获取限时停车规则
    GET_ILLEGAL_PK_RULE,//获取限时违停规则
    UPLOAD_LOG_INFO,    //上传日志url
    AUTO_UPGRADE        //自动升级
}EOCCLOUD_TASK;

typedef struct EOCCloudData{
    bool is_running;                    //线程运行标志
    char file_server_path[64];
    int file_server_port;
    
    vector<EocDownloadsMsg> downloads_msg;
    vector<EocUpgradeDev> upgrade_msg;
    vector<EOCCLOUD_TASK> task;

    pthread_mutex_t eoc_msg_mutex;
}EOCCloudData;



/*******上传eoc实时图片***********************/
//图片要素点
typedef struct
{
    string LaneGuid; //车道guid
    string Id; //识别id
    string Longitude; //GPS坐标经度
    string Latitude;  //GPS坐标纬度
    int LeftUpX;    //左上 X
    int LeftUpY;    //左上 Y
    int RightDownX;   //右下X
    int RightDownY;   //右下Y
    int CenterX;    //中心点X
    int CenterY;    //中心点Y
    float LocationX;    //对于雷达来说，x方向的距离
    float LocationY;    //对于雷达来说，y方向的距离
}Pic_Element;
//雷达要素点
typedef struct
{
    string LaneGuid; //车道guid
    string Id; //识别id
    string Longitude; //GPS坐标经度
    string Latitude;  //GPS坐标纬度
    float LocationX;    //对于雷达来说，x方向的距离
    float LocationY;    //对于雷达来说，y方向的距离
}Radar_Element;
//实时图片
typedef struct
{
    string PicPath; //图片地址
    std::vector <Pic_Element> Elements; //图片要素点，ImgElements
    std::vector <Radar_Element> RadarElements; //雷达要素点
    std::vector <Pic_Element> FusionElements; //融合要素点
    string TimeStamp;   //时间戳 yyyyMMddHHmmss
    string CameraGuid;  //相机guid
}Real_Time_Pic_Info;
//开启实时图片上传相机信息
typedef struct
{
    string CameraGuid;
    string CameraIp;
    tcp_client_t* TcpInfo;   //通信链接信息
    string CommGuid;    //通信编号
    int Enable;         //是否上传标志，0停止上传，1上传
}Real_Time_Camera_Info;


/*******上传eoc状态***********************/
//相机状态
typedef struct
{
    string Guid; //相机 Guid
    int State; //相机状态：0=正常，1异常
    string Model; //相机型号： 例如 hikv01、tps01
    string SoftVersion; //相机版本
    string DataVersion; //相机详细配置数据版本——忽略
    int GetPicTotalCount; //相机取图总次数
    int GetPicSuccessCount; //取图成功次数
}UP_EOC_CameraState;
//雷达状态
typedef struct
{
    string Guid; //雷达 Guid
    int State; //雷达状态：0=正常，1异常
    string SoftVersion; //固件版本
}UP_EOC_RadarState;

int update_camera_state(UP_EOC_CameraState data);
/***更新雷达状态
** radar_guid:雷达 Guid
** state:雷达状态：0=正常，1异常
** soft_version:固件版本
**/
int update_radar_state(const char* radar_guid, int state, const char* soft_version);



int eoc_communication_start(const char *server_path, int server_port);

//添加下载任务
//返回值：0添加成功；-1添加失败
int add_eoc_download_event(EocDownloadsMsg &data, EocUpgradeDev &dev_data);
//添加升级任务
//返回值：0添加成功；-1添加失败
int add_eoc_upgrade_event(EocUpgradeDev &data);


/*
 * 开始实时图片获取返回
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int s104_start_real_time_pic_send(tcp_client_t* client, const char *comm_guid, Real_Time_Pic_Info &info_data);
int get_upload_pic_camera(vector <Real_Time_Camera_Info> &data);
//返回相机是否继续上传实时图，1继续上传，0停止上传
int get_upload_camera_state(const char* camera_ip);




#endif
