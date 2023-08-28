//
// Created by lining on 11/4/22.
//

#include "Eoc.h"
#include <glog/logging.h>


#include "eoc_comm/sqlite_eoc_db.h"
#include "eoc_comm/eoc_comm.hpp"
#include "eoc_comm/configure_eoc_init.h"
#include "eoc_comm/utility/dns_server.h"

//在登录EOC的时候，主要是获取 HOME_PATH"/bin/RoadsideParking.db"内的 TB_ParkingLot表的 CloudServerPath、CloudServerPort 用于登录EOC

int StartEocCommon() {
    dns_server_start();  /*dns服务*/
    if (g_eoc_config_init() < 0) {
        printf("g_eoc_config_init err\n");
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "116.63.162.151";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    int ret = db_parking_lot_get_cloud_addr_from_factory(eoc_host, &eoc_port, file_host, &file_port);
    if (ret == 0) {
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n", eoc_host.c_str(), eoc_port);
    }
    if (eoc_port == 0) {
        eoc_port = 6526;
    }
    if (eoc_host.empty()) {
        eoc_host = "116.63.162.151";
    }
    string ipaddr;
//    url_get(eoc_host,ipaddr);
    eoc_communication_start(eoc_host.c_str(), eoc_port);//主控机端口固定为6526
}

#include "eocCom/EOCCom.h"
#include "eocCom/dns/DNSServer.h"
#include "eocCom/db/DBCom.h"

static void ThreadEOCCom(std::string ip, int port, std::string cert) {
    LOG(INFO) << "eoc thread:" << ip << ":" << port << "cert:" << cert;
    EOCCom *eocCom = new EOCCom(ip, port, cert);
//    EOCCom *eocCom = new EOCCom("127.0.0.1",8000,"./cert.pem");
    if (eocCom->Open() == 0) {
        eocCom->Run();
    }
    while (true) {
        sleep(60);
        if (!eocCom->isLive) {
            eocCom->Close();
            if (eocCom->Open() == 0) {
                LOG(INFO) << "eoc thread restart eocCom";
                eocCom->Run();
            }
        }
    }
    eocCom->Close();
    delete eocCom;
}

int StartEocCommon1() {
    myDNS::DNSServerStart();  /*dns服务*/
    if (globalConfigInit() < 0) {
        LOG(ERROR) << "g_eoc_config_init err";
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "116.63.162.151";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    int ret = dbGetCloudInfo(eoc_host, eoc_port, file_host, file_port);
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