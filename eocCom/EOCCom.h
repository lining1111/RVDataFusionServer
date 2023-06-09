//
// Created by lining on 2023/2/23.
//

#ifndef EOCCOM_H
#define EOCCOM_H

#include "Queue.h"
#include "net/tlsClient/TlsClient.h"
#include "EOCJSON.h"
#include "db/DBTable.h"
/**
 * EOC 通信类
 */

class EOCCom : public TlsClient {

public:
    bool businessStart = false;
    Queue<std::string> pkgs;//分包
    //一些通信包
    std::future<int> future_getPkgs;
    std::future<int> future_proPkgs;
    std::future<int> future_interval;
    bool isLogIn = false;
    int heartFlag = 0;//每次发送加1,每次接到回复-1,如果加到10,则表明10次没有得到回复，就要重连
    time_t last_login_t = time(nullptr);//上次登录时间
    time_t last_send_heart_t = time(nullptr);//上次发送心态时间
    time_t last_send_net_state_t = time(nullptr);//上次发送外围状态时间
    time_t last_get_config_t = time(nullptr);//上次获取配置时间

    int sendNetTotal = 0;
    int sendNetSuccess = 0;

    //下载更新相关
    enum EocDownloadStatus {
        DOWNLOAD_IDLE = 1,
        DOWNLOAD_RUNNING,     //暂未用
        DOWNLOAD_FINISHED,
        DOWNLOAD_FILE_NOT_EXIST,
        DOWNLOAD_TIMEOUT,
        DOWNLOAD_SPACE_NOT_ENOUGH,
        DOWNLOAD_MD5_FAILD,
        DOWNLOAD_WATI_UPGRADE
    };
    enum EocUpgradeDevType {
        EOC_UPGRADE_PARKINGAREA = 1,  //主控板升级
        EOC_UPGRADE_CAMARA,           //相机升级
        EOC_UPGRADE_CONTROLLER        //矩阵控制器升级
    };
    enum EocUpgradeStatus {
        EOC_UPGRADE_IDLE = 1, //初始添加升级
        EOC_UPGRADE_RUNNING,  //升级中
        EOC_UPGRADE_SUCCESS,  //升级成功
        EOC_UPGRADE_FAILED    //升级失败
    };

    typedef struct {
        std::string dev_guid;
        std::string dev_ip;
        EocUpgradeDevType dev_type;
        time_t start_upgrade_time;
        std::string sw_version;
        std::string hw_version;
        std::string dev_model;   //设备型号——暂只用于相机
        int upgrade_mode;   //升级方式1选择升级；2强制升级
        std::string upgrade_time;
        std::string comm_guid;   //通信guid
        EocUpgradeStatus up_status; //升级状态
        int up_progress;    //升级进度 0~100：状态为开始升级->进度进到10，升级中->10~99，升级完成(成功/失败)->100
    } EocUpgradeDev;

    typedef struct {
        std::vector<EocUpgradeDev> upgrade_dev;
        EocDownloadStatus download_status;
//        pthread_t download_thread_id;
        thread::id download_thread_id;
        std::string download_url;
        std::string download_file_name;
        int download_file_size;
        std::string download_file_md5;
//    string comm_guid;
    } EocDownloadsMsg;

    enum EOCCLOUD_TASK {
        SYS_REBOOT = 0,     //重启进程
        GET_PK_RULE,        //获取限时停车规则
        GET_ILLEGAL_PK_RULE,//获取限时违停规则
        UPLOAD_LOG_INFO,    //上传日志url
        AUTO_UPGRADE        //自动升级
    };

    typedef struct {
        bool is_running;                    //线程运行标志
        std::string file_server_path;
        int file_server_port;
        std::vector<EocDownloadsMsg> downloads_msg;
        std::vector<EocUpgradeDev> upgrade_msg;
        std::vector<EOCCLOUD_TASK> task;

        pthread_mutex_t eoc_msg_mutex;
    } EOCCloudData;

    //
    EOCCloudData  eocCloudData;

public:
    EOCCom(string ip, int port, string caPath);

    ~EOCCom() {

    }

    int Open();

    int Run();

    int Close();

private:
    static int getPkgs(void *p);

    static int proPkgs(void *p);

    static int intervalPro(void *p);

public:
    static void convertBaseSetS2DB(class BaseSettingEntity in,DBBaseSet &out,int index);

    static void convertIntersectionS2DB(class IntersectionEntity in, DBIntersection &out);

    static void convertFusion_Para_SetS2DB(class FusionParaEntity in, DBFusionParaSet &out);

    static void convertAssociated_EquipS2DB(class AssociatedEquip in,DBAssociatedEquip &out);
};


#endif //EOCCOM_H
