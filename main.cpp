#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include <gflags/gflags.h>
#include <csignal>
#include "localBussiness/localBusiness.h"
#include "httpServer/httpServer.h"
#include "eoc/Eoc.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "db/DB.h"

using namespace z_log;

int signalIgnpipe() {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) < 0) {
        Error("call sigaction fail, %s\n", strerror(errno));
        return errno;
    }

    return 0;
}

DEFINE_int32(port, 9000, "本地服务端端口号，默认9000");
DEFINE_string(cloudIp, "10.110.25.149", "云端ip，默认 10.110.25.149");
DEFINE_int32(cloudPort, 7890, "云端端口号，默认7890");
DEFINE_int32(keep, 604800, "云端端口号，默认604800");
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");

int main(int argc, char **argv) {

    StartEocCommon();

    google::SetVersionString(VERSION_BUILD_TIME);
    google::ParseCommandLineFlags(&argc, &argv, true);
    z_log::init(PROJECT_NAME, FLAGS_keep);
    uint16_t port = FLAGS_port;
    string cloudIp;

    if (!string(g_eoc_base_set.PlatformTcpPath).empty()) {
        cloudIp = string(g_eoc_base_set.PlatformTcpPath);
        Notice("采用数据库配置,cloud ip:%s", cloudIp.c_str());
    } else {
        cloudIp = FLAGS_cloudIp;
        Notice("采用程序参数配置,cloud ip:%s", cloudIp.c_str());
    }

    uint16_t cloudPort;

    if (g_eoc_base_set.PlatformTcpPort != 0) {
        cloudPort = g_eoc_base_set.PlatformTcpPort;
        Notice("采用数据库配置,cloud port:%d", cloudPort);
    } else {
        cloudPort = FLAGS_cloudPort;
        Notice("采用程序参数配置,cloud port:%d", cloudPort);
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

    z_log::finish();

    return 0;
}
