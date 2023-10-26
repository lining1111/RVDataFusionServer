//
// Created by lining on 11/4/22.
//

#include "Eoc.h"
#include <glog/logging.h>
#include "common/config.h"
#include "os/os.h"
#include "eocCom/EOCCom.h"
#include "eocCom/dns/DNSServer.h"
#include "eocCom/DBCom.h"

//在登录EOC的时候，主要是获取 HOME_PATH"/bin/RoadsideParking.db"内的 TB_ParkingLot表的 CloudServerPath、CloudServerPort 用于登录EOC

#define EocCommonVersion "1.0.6"

string GetEocCommonVersion(){
    return EocCommonVersion;
}

static void ThreadEOCCom(std::string ip, int port, std::string cert) {
    LOG(WARNING) << "eoc thread:" << ip << ":" << port << "cert:" << cert;
    EOCCom *eocCom = new EOCCom(ip, port, cert);
//    EOCCom *eocCom = new EOCCom("127.0.0.1",8000,"./cert.pem");
    if (eocCom->Open() == 0) {
        eocCom->Run();
    }
    while (true) {
        sleep(10);
        if (!eocCom->isLive.load()) {
            LOG(WARNING) << "eoc thread restart eocCom";
            eocCom->Close();
            if (eocCom->Open() == 0) {
                eocCom->Run();
                LOG(WARNING) << "eoc thread restart eocCom success";
            }
        }
    }
    eocCom->Close();
    delete eocCom;
}

int StartEocCommon() {
    myDNS::DNSServerStart();  /*dns服务*/
    if (globalConfigInit() < 0) {
        LOG(ERROR) << "g_eoc_config_init err";
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "116.63.162.151";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    int ret = getEOCInfo(eoc_host, eoc_port, file_host, file_port);
    if (ret == 0) {
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n", eoc_host.c_str(), eoc_port);
    }
    if (eoc_port == 0) {
        eoc_port = 6526;
    }
    if (eoc_host.empty()) {
        eoc_host = "116.63.162.151";
    }
    if (myDNS::isIP((char *) eoc_host.c_str()) == 0) {
        string ipaddr;
        myDNS::url_get(eoc_host, ipaddr);
        eoc_host = ipaddr;
    }
    std::thread(ThreadEOCCom, eoc_host, eoc_port, "./eoc.pem").detach();
    return 0;
}

#define ASS(index, array) \
    if(index>array.size()){ \
        return 0;            \
    }

static int getNTPFromContent(string &ip, int &port, string content) {
    /*
		"* get_ntp_info $ntpServer 123  *"
	*/
    if (content.empty()) {
        return -1;
    }
    //先分割字符
    vector<string> strs = os::split(content, " ");
    if (strs.empty()) {
        return -1;
    }
    //找到 get_ntp_info
    int index = -1;
    for (int i = 0; i < strs.size(); i++) {
        auto iter = strs.at(i);
        if (iter == "get_ntp_info") {
            index = i + 1;
            break;
        }
    }
    if (index == -1 || index > strs.size()) {
        return -1;
    }

    //写入信息
    ip = strs.at(index);
    index++;
    ASS(index, strs)
    port = strtol(strs.at(index).c_str(), nullptr, 10);
    return 0;
}

static int GetNTPInfo(string &ServerIp, int &ServerPort) {
    int ret = 0;
    string cmd = homePath + "/bin/get_ntp_info";
    string result;
    os::runCmd(cmd, &result);
    LOG(INFO) << "shell " << cmd << "result:" << result;
    if (!result.empty()) {
        if (getNTPFromContent(ServerIp, ServerPort, result) == 0) {

        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }
    return ret;
}


static int SetNTPInfo(string ServerIp, int ServerPort) {
    int ret = 0;
    string ip = ServerIp;
    string port = to_string(ServerPort);
    string cmd = homePath + "/bin/set_ntp_info ";
    cmd += ip;
    cmd += " ";
    cmd += port;
    string result;
    os::runCmd(cmd, &result);
    LOG(INFO) << "shell " << cmd << "result:" << result;
    //TODO 判断执行结果
    return ret;
}


int SetNtpServer(string serverIp) {
    if (serverIp.empty()) {
        LOG(ERROR) << "ntp serverIp is empty";
        return -1;
    }
    LOG(WARNING) << "ntp set serverIp:" << serverIp;
    //1.获取原先的ip
    string oriServerIp;
    int oriServerPort;
    if (GetNTPInfo(oriServerIp, oriServerPort) == 0) {
        LOG(WARNING) << "ntp info get success,oriServerIp:" << oriServerIp << ",oriServerPort:" << oriServerPort;
        if (oriServerIp != serverIp) {
            LOG(WARNING) << "ntp oriServerIp:" << oriServerIp << "!=serverIp:" << serverIp << ",set to " << serverIp;
            if (SetNTPInfo(serverIp, oriServerPort) == 0) {
                LOG(WARNING) << "ntp set success:" << serverIp << ":" << oriServerPort;
                return 0;
            } else {
                return -1;
            }
        }
        return 0;
    } else {
        LOG(ERROR) << "ntp info get error";
        return -1;
    }
    return 0;
}
