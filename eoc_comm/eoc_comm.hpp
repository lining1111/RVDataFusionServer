#ifndef __DMCJ_EOC_COMM_HPP__
#define __DMCJ_EOC_COMM_HPP__

//nx编译 g++ -o zkj_eoc_test src/*.cpp src/utility/*.cpp src/utility/*.c -Isrc/ -Isrc/utility/ -std=c++11 -g -lssl -lcrypto -lpthread -L/usr/local/lib -I/usr/local/include -lsqlite3 -lcurl

#include <string>
#include <vector>
#include "ttcp.hpp"

using namespace std;


#define EOC_COMMUNICATION_VERSION "V1.0.0"  //eoc通信协议版本号


/**********EOC下载、升级***********/
typedef enum EocDownloadStatus {
    DOWNLOAD_IDLE = 1,
    DOWNLOAD_RUNNING,     //暂未用
    DOWNLOAD_FINISHED,
    DOWNLOAD_FILE_NOT_EXIST,
    DOWNLOAD_TIMEOUT,
    DOWNLOAD_SPACE_NOT_ENOUGH,
    DOWNLOAD_MD5_FAILD,
    DOWNLOAD_WATI_UPGRADE
} EocDownloadStatus;
typedef enum EocUpgradeDevType {
    EOC_UPGRADE_PARKINGAREA = 1,  //主控板升级
    EOC_UPGRADE_CAMARA,           //相机升级
    EOC_UPGRADE_CONTROLLER        //矩阵控制器升级
} EocUpgradeDevType;
typedef enum EocUpgradeStatus {
    EOC_UPGRADE_IDLE = 1, //初始添加升级
    EOC_UPGRADE_RUNNING,  //升级中
    EOC_UPGRADE_SUCCESS,  //升级成功
    EOC_UPGRADE_FAILED    //升级失败
} EocUpgradeStatus;

typedef struct EocUpgradeDev_t {
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
} EocUpgradeDev;

typedef struct EocDownloadsMsg_t {
    vector<EocUpgradeDev> upgrade_dev;
    EocDownloadStatus download_status;
    pthread_t download_thread_id;
    string download_url;
    string download_file_name;
    int download_file_size;
    string download_file_md5;
//    string comm_guid;
} EocDownloadsMsg;

typedef enum EOCCLOUD_TASK {
    SYS_REBOOT = 0,     //重启进程
    GET_PK_RULE,        //获取限时停车规则
    GET_ILLEGAL_PK_RULE,//获取限时违停规则
    UPLOAD_LOG_INFO,    //上传日志url
    AUTO_UPGRADE        //自动升级
} EOCCLOUD_TASK;

typedef struct EOCCloudData {
    bool is_running;                    //线程运行标志
    char file_server_path[64];
    int file_server_port;

    vector<EocDownloadsMsg> downloads_msg;
    vector<EocUpgradeDev> upgrade_msg;
    vector<EOCCLOUD_TASK> task;

    pthread_mutex_t eoc_msg_mutex;
} EOCCloudData;


int eoc_communication_start(const char *server_path, int server_port);

//添加下载任务
//返回值：0添加成功；-1添加失败
int add_eoc_download_event(EocDownloadsMsg &data, EocUpgradeDev &dev_data);

//添加升级任务
//返回值：0添加成功；-1添加失败
int add_eoc_upgrade_event(EocUpgradeDev &data);

//主控机状态
typedef struct {
    int State = 1;
    int Size = 0;
    int ResidualSize = 0;
} PartsState;

typedef struct {
    string MainboardGuid;
    int State = 1;
    int CpuState = 1;
    double CpuUtilizationRatio = 0.0;
    double CpuTemperature = 0.0;
    int MemorySize = 0;
    int ResidualMemorySize = 0;
    string ModelVersion;
    string MainboardType;
    vector<PartsState> TFCardStates;
    vector<PartsState> EmmcStates;
    vector<PartsState> ExternalHardDisk;
} MainBoardState;

//获取主控机状态
//返回值：0获取成功；-1获取失败
int GetMainBoardState(MainBoardState &mainBoardState);

#endif
