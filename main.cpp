#include <unistd.h>
#include <fcntl.h>
#include "version.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <csignal>
#include "localBussiness/localBusiness.h"

#include "eoc/Eoc.h"
#include "eocCom/EOCCom.h"
#include "eocCom/db/DBCom.h"
#include "data/Data.h"

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include "glogHelper/GlogHelper.h"
#include "common/config.h"

int signalIgnPipe() {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) < 0) {
        printf("call sigaction fail, %s\n", strerror(errno));
        return errno;
    }

    return 0;
}

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "10.110.60.122", "云端ip，默认 10.110.60.122");
DEFINE_int32(cloudPort, 9988, "云端端口号，默认9988");
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");
DEFINE_int32(mergeMode, 0, "多路融合模式，默认0,0:雷视 1:雷达 2:图像");
DEFINE_int32(keep, 5, "日志清理周期 单位day，默认5");
DEFINE_bool(isSendPIC, true, "发送图片到云端，默认true");
DEFINE_bool(isSendSTDOUT, false, "输出到控制台，默认false");
DEFINE_int32(roadNum, 4, "外设路数，默认4");
#ifdef aarch64
DEFINE_string(logDir, "/mnt/mnt_hd", "日志的输出目录,默认/mnt/mnt_hd");
#else
DEFINE_string(logDir, "/tmp", "日志的输出目录,默认/tmp");
#endif

#include "eocCom/fileFun.h"

int main(int argc, char **argv) {

    char curPath[512];
    getcwd(curPath, 512);
    printf("cur path:%s\n", curPath);
    if (opendir(FLAGS_logDir.c_str()) == nullptr) {
        if (mkdir(FLAGS_logDir.c_str(), 0644)) {
            printf("create %s fail\n", FLAGS_logDir.c_str());
        }
    }

    gflags::SetVersionString(VERSION_BUILD_TIME);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::string proFul = std::string(argv[0]);
    std::string pro = os::g_getFileName(proFul);

    //日志系统类
    GlogHelper glogHelper(pro, FLAGS_keep, FLAGS_logDir, FLAGS_isSendSTDOUT);

    uint16_t port = FLAGS_port;
    string cloudIp;
    uint16_t cloudPort;

    //初始化本地数据和数据库
    LOG(INFO) << "开启eoc通信，同时读取本地数据库到缓存";
//    StartEocCommon();
//    if (!string(g_eoc_base_set.PlatformTcpPath).empty()) {
//        cloudIp = string(g_eoc_base_set.PlatformTcpPath);
//        LOG(INFO) << "采用数据库配置,cloud ip:" << cloudIp;
//    } else {
//        cloudIp = FLAGS_cloudIp;
//        LOG(INFO) << "采用程序参数配置,cloud ip:" << cloudIp;
//    }
//
//    if (g_eoc_base_set.PlatformTcpPort != 0) {
//        cloudPort = g_eoc_base_set.PlatformTcpPort;
//        LOG(INFO) << "采用数据库配置,cloud port:" << cloudPort;
//    } else {
//        cloudPort = FLAGS_cloudPort;
//        LOG(INFO) << "采用程序参数配置,cloud port:" << cloudPort;
//    }

//    StartEocCommon1();
    if (!string(g_BaseSet.PlatformTcpPath).empty()) {
        cloudIp = string(g_BaseSet.PlatformTcpPath);
        LOG(INFO) << "采用数据库配置,cloud ip:" << cloudIp;
    } else {
        cloudIp = FLAGS_cloudIp;
        LOG(INFO) << "采用程序参数配置,cloud ip:" << cloudIp;
    }

    if (g_BaseSet.PlatformTcpPort != 0) {
        cloudPort = g_BaseSet.PlatformTcpPort;
        LOG(INFO) << "采用数据库配置,cloud port:" << cloudPort;
    } else {
        cloudPort = FLAGS_cloudPort;
        LOG(INFO) << "采用程序参数配置,cloud port:" << cloudPort;
    }

    //将配置写入
    localConfig.isSendPIC.push_back(ConfigEnable{cloudIp, cloudPort, FLAGS_isSendPIC});
    // localConfig.mergeMode = FLAGS_mergeMode;
    localConfig.mergeMode = 2;
    localConfig.roadNum = FLAGS_roadNum;
    auto dataLocal = Data::instance();
    dataLocal->isMerge = FLAGS_isMerge;
    LOG(INFO) << "初始化本地数据，Data地址:" << dataLocal->m_pInstance;
    LOG(INFO) << "开启本地tcp通信，包括本地服务端和连接上层的客户端";
    signalIgnPipe();
    LocalBusiness local;
    local.AddServer("server1", port);
    local.AddServer("server2", port + 1);
    local.AddClient("client1", cloudIp, cloudPort);
    //增加一个默认端口的客户端
    local.AddClient("client2", cloudIp, fixrPort);
    //开启本地业务
    local.Run();

//    FusionServer *server = new FusionServer(port);
//    if (server->Open() == 0) {
//        server->Run();
//    }
//    sleep(1);
//    delete server;

    while (true) {
        sleep(5);
    }

    return 0;
}
