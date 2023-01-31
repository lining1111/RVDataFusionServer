#include <unistd.h>
#include "version.h"
#include <gflags/gflags.h>
#include "glog/logging.h"
#include <csignal>
#include "localBussiness/localBusiness.h"
#include "httpServer/httpServer.h"
#include "eoc/Eoc.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "db/DB.h"

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

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "139.9.157.176", "云端ip，默认 139.9.157.176");
DEFINE_int32(cloudPort, 3410, "云端端口号，默认3410");
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");
DEFINE_int32(keep, 7, "日志清理周期 单位day，默认7");
DEFINE_string(logDir, "/mnt/mnt_hd/log", "日志的输出目录,默认/mnt/mnt_hd/log");

int main(int argc, char **argv) {
    gflags::SetVersionString(VERSION_BUILD_TIME);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::SetLogFilenameExtension(".log");
    google::FlushLogFiles(google::INFO);
    FLAGS_log_dir = FLAGS_logDir;
    google::EnableLogCleaner(FLAGS_keep);
    uint16_t port = FLAGS_port;
    string cloudIp;

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
        LOG(INFO) << "采用程序参数配置,cloud port:%d" << cloudPort;
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
    StartEocCommon();
    while (true) {
        sleep(5);
    }

    google::ShutdownGoogleLogging();
    return 0;
}
