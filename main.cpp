#include <unistd.h>
#include "version.h"
#include <gflags/gflags.h>
#include "glog/logging.h"
#include <csignal>
#include "localBussiness/localBusiness.h"
#include "eoc/Eoc.h"

#include <fstream>
#include "db/DB.h"
#include "logFileCleaner/LogFileCleaner.h"

int signalIgnpipe() {
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

void FatalMessageDump(const char *data, unsigned long size) {
    std::ofstream fs("./fatal.log", std::ios::app);
    std::string str = std::string(data, size);
    fs << str;
    fs.close();
    LOG(INFO) << str;
}

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "139.9.157.176", "云端ip，默认 139.9.157.176");
DEFINE_int32(cloudPort, 3410, "云端端口号，默认3410");
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");
DEFINE_int32(keep, 1, "日志清理周期 单位day，默认1");
DEFINE_bool(isSendSTDOUT, false, "输出到控制台，默认false");
#ifdef aarch64
DEFINE_string(logDir, "/mnt/mnt_hd", "日志的输出目录,默认/mnt/mnt_hd");
#else
DEFINE_string(logDir, "log", "日志的输出目录,默认log");
#endif

int main(int argc, char **argv) {
    gflags::SetVersionString(VERSION_BUILD_TIME);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureWriter(&FatalMessageDump);
    FLAGS_log_dir = FLAGS_logDir;
    google::EnableLogCleaner(FLAGS_keep);
    FLAGS_logbufsecs = 0;//刷新日志buffer的时间，0就是立即刷新
    FLAGS_stop_logging_if_full_disk = true; //设置是否在磁盘已满时避免日志记录到磁盘
    if (FLAGS_isSendSTDOUT) {
        FLAGS_logtostdout = true;
    }
    //开启日志清理类
    std::string _name = string(argv[0])+".";
    LogFileCleaner *cleaner = new LogFileCleaner(_name,FLAGS_logDir,(60*60*24)*FLAGS_keep);

    uint16_t port = FLAGS_port;
    string cloudIp;
    StartEocCommon();
    if (!string(g_eoc_base_set.PlatformTcpPath).empty()) {
        cloudIp = string(g_eoc_base_set.PlatformTcpPath);
        LOG(INFO) << "采用数据库配置,cloud ip:" << cloudIp;
    } else {
        cloudIp = FLAGS_cloudIp;
        LOG(INFO) << "采用程序参数配置,cloud ip:" << cloudIp;
    }

    uint16_t cloudPort;

    if (g_eoc_base_set.PlatformTcpPort != 0) {
        cloudPort = g_eoc_base_set.PlatformTcpPort;
        LOG(INFO) << "采用数据库配置,cloud port:" << cloudPort;
    } else {
        cloudPort = FLAGS_cloudPort;
        LOG(INFO) << "采用程序参数配置,cloud port:" << cloudPort;
    }

    bool isMerge = FLAGS_isMerge;

    signalIgnpipe();
    LocalBusiness local;
    local.AddServer("server1", port, isMerge);
    local.AddServer("server2", port + 1, isMerge);
    local.AddClient("client1", cloudIp, cloudPort, nullptr);

    //开启本地业务
    local.Run();

//    FusionServer *server = new FusionServer(port, isMerge);
//    if (server->Open() == 0) {
//        server->Run();
//    }
//    sleep(1);
//    delete server;

//    HttpServerInit(10000, &local);

    while (true) {
        sleep(5);
    }

    google::ShutdownGoogleLogging();
    delete cleaner;

    return 0;
}
