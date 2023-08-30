#include <unistd.h>
#include <fcntl.h>
#include "version.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <csignal>
#include "localBussiness/localBusiness.h"

#include "eoc/Eoc.h"
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
DEFINE_bool(isSamePort, false, "是否发送同一端口，默认false");
DEFINE_bool(isMerge, false, "是否融合多路数据，默认false");
DEFINE_int32(mergeMode, 0, "多路融合模式，默认0,0:雷视 1:雷达 2:图像");
DEFINE_int32(keep, 5, "日志清理周期 单位day，默认5");
DEFINE_bool(isSendPIC, true, "发送图片到云端，默认true");
DEFINE_bool(isSendSTDOUT, false, "输出到控制台，默认false");
DEFINE_int32(roadNum, 8, "外设路数，默认8");
DEFINE_string(logDir, "/mnt/mnt_hd", "日志的输出目录,默认/mnt/mnt_hd");

DEFINE_bool(isSaveInObj, false, "存算法输入，默认false");
DEFINE_bool(isSaveOutObj, false, "存算法输出，默认false");

DEFINE_string(algorithmParamFile, "./algorithmParam.json", "算法配置文件,默认./algorithmParam.json");

#include "configure_eoc_init.h"
#include "os/os.h"
#include "eocCom/db/DBCom.h"

int main(int argc, char **argv) {

    char *curPath;
    curPath = new char[512];
    getcwd(curPath, 512);
    printf("cur path:%s\n", curPath);
    LOG(WARNING) << "程序工作目录:" << string(curPath) << ",版本号:" << VERSION_BUILD_TIME;
    delete[]curPath;

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
    LOG(WARNING) << "开启eoc通信，同时读取本地数据库到缓存";

    bool isUseOldEOC = true;

    if (isUseOldEOC) {
        StartEocCommon();
        if (!string(g_eoc_base_set.PlatformTcpPath).empty()) {
            cloudIp = string(g_eoc_base_set.PlatformTcpPath);
            LOG(WARNING) << "xh采用数据库配置,cloud ip:" << cloudIp;
        } else {
            cloudIp = FLAGS_cloudIp;
            LOG(WARNING) << "xh采用程序参数配置,cloud ip:" << cloudIp;
        }

        if (g_eoc_base_set.PlatformTcpPort != 0) {
            cloudPort = g_eoc_base_set.PlatformTcpPort;
            LOG(WARNING) << "xh采用数据库配置,cloud port:" << cloudPort;
        } else {
            cloudPort = FLAGS_cloudPort;
            LOG(WARNING) << "xh采用程序参数配置,cloud port:" << cloudPort;
        }

    } else {
        StartEocCommon1();
        if (!string(g_BaseSet.PlatformTcpPath).empty()) {
            cloudIp = string(g_BaseSet.PlatformTcpPath);
            LOG(WARNING) << "采用数据库配置,cloud ip:" << cloudIp;
        } else {
            cloudIp = FLAGS_cloudIp;
            LOG(WARNING) << "采用程序参数配置,cloud ip:" << cloudIp;
        }

        if (g_BaseSet.PlatformTcpPort != 0) {
            cloudPort = g_BaseSet.PlatformTcpPort;
            LOG(WARNING) << "采用数据库配置,cloud port:" << cloudPort;
        } else {
            cloudPort = FLAGS_cloudPort;
            LOG(WARNING) << "采用程序参数配置,cloud port:" << cloudPort;
        }
    }
    //将配置写入
    localConfig.isSendPIC = FLAGS_isSendPIC;
    localConfig.mergeMode = FLAGS_mergeMode;
    localConfig.roadNum = FLAGS_roadNum;
    localConfig.isSaveInObj = FLAGS_isSaveInObj;
    localConfig.isSaveOutObj = FLAGS_isSaveOutObj;

    //读取默认的算法配置文件
    if (getAlgorithmParam(FLAGS_algorithmParamFile, localConfig.algorithmParam) != 0) {
        LOG(ERROR) << "读取算法配置文件失败:" << FLAGS_algorithmParamFile;
    }
    LOG(WARNING) << "通信协议版本:" << GetComVersion();

    auto dataLocal = Data::instance();
    dataLocal->isMerge = FLAGS_isMerge;
    LOG(WARNING) << "初始化本地数据，Data地址:" << dataLocal->m_pInstance;
    LOG(WARNING) << "开启本地tcp通信，包括本地服务端和连接上层的客户端";
    signalIgnPipe();
    auto businessLocal = LocalBusiness::instance();
    businessLocal->AddServer("server1", port);
    businessLocal->AddServer("server2", port + 1);
    businessLocal->AddClient("client1", cloudIp, cloudPort);
    //增加一个默认端口的客户端
    if (!FLAGS_isSamePort) {
        businessLocal->AddClient("client2", cloudIp, fixrPort);
    } else {
        businessLocal->AddClient("client2", cloudIp, cloudPort);
    }
    //开启本地业务
    businessLocal->Run();

    //信控机测试，打开信控机
    signalControl = new SignalControl("172.16.1.1", 15050, 12345);
    if (signalControl->Open() != 0) {
        //打开失败
        LOG(ERROR) << "信控机打开失败";
        delete signalControl;
    }

//    FusionServer *server = new FusionServer(port);
//    if (server->Open() == 0) {
//        server->Run();
//    }
//    sleep(1);
//    delete server;

    while (true) {
        sleep(5);
        if (dataLocal->isStartFusion && 0) {
            //判断是否有10s没有发送数据了
            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            if ((now - dataLocal->dataUnitFusionData->curTimestamp) > (10 * 1000)) {
                LOG(ERROR) << "data fusion not send 10s";
                exit(-1);
            }

        }
    }
    delete businessLocal;
    delete dataLocal;
    return 0;
}
