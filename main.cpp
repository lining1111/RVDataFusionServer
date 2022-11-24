#include <unistd.h>
#include "log/Log.h"
#include "version.h"
#include <gflags/gflags.h>
#include <csignal>
#include "localBusiness.h"
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
DEFINE_bool(isMerge, true, "是否融合多路数据，默认true");

int main(int argc, char **argv) {

    if (0) {
        string dirName1 = "mergeData";
        int isCreate1 = mkdir(dirName1.data(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (!isCreate1)
            Info("create path:%s\n", dirName1.c_str());
        else
            Info("create path failed! error code : %d \n", isCreate1);
    }

    google::SetVersionString(PROJECT_VERSION);
    google::ParseCommandLineFlags(&argc, &argv, true);
    z_log::init();
    uint16_t port = FLAGS_port;
    string cloudIp = FLAGS_cloudIp;
    uint16_t cloudPort = FLAGS_cloudPort;
    bool isMerge = FLAGS_isMerge;

    signalIgnpipe();
    LocalBusiness local;
    local.AddServer("server1", port, isMerge);
    local.AddServer("server2", port + 1, isMerge);
    local.AddClient("client1", cloudIp, cloudPort, nullptr);

    //开启本地业务
    local.Run();

//    HttpServerInit(10000, &local);

//    StartEocCommon();
    while (true) {
        sleep(10);
    }

    return 0;
}
