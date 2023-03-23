//
// Created by lining on 11/4/22.
//

#include "Eoc.h"
#include <glog/logging.h>


#include "eoc_comm/sqlite_eoc_db.h"
#include "eoc_comm/eoc_comm.hpp"
#include "eoc_comm/configure_eoc_init.h"
#include "eoc_comm/utility/dns_server.h"
int StartEocCommon() {
    dns_server_start();  /*dns服务*/
    if (g_eoc_config_init() < 0) {
        printf("g_eoc_config_init err\n");
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "ehctest.eoc.aipark.com";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    string ipaddr = "116.63.162.151"/*"111.62.28.98"*/;
    int ret = db_parking_lot_get_cloud_addr_from_factory(eoc_host, &eoc_port, file_host, &file_port);
    if (ret == 0) {
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n", eoc_host.c_str(), eoc_port);
    }
    eoc_port = 6526;
    eoc_host = "ehctest.eoc.aipark.com";
//    url_get(eoc_host,ipaddr);
    eoc_communication_start(ipaddr.c_str(), 6526);
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
    DNSServerStart();  /*dns服务*/
    if (globalConfigInit() < 0) {
        LOG(ERROR) << "g_eoc_config_init err";
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "ehctest.eoc.aipark.com";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    string ipaddr = "116.63.162.151"/*"111.62.28.98"*/;
    int ret = dbGetCloudInfo(eoc_host, eoc_port, file_host, file_port);
    if (ret == 0) {
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n", eoc_host.c_str(), eoc_port);
    }
    eoc_port = 6526;
    eoc_host = "ehctest.eoc.aipark.com";
//    url_get(eoc_host,ipaddr);
    std::thread(ThreadEOCCom, ipaddr, 6524, "./eoc.pem").detach();
    return 0;
}