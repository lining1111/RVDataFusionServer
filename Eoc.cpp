//
// Created by lining on 11/4/22.
//

#include "Eoc.h"
#include "eoc_comm/sqlite_eoc_db.h"
#include "eoc_comm/eoc_comm.hpp"
#include "eoc_comm/configure_eoc_init.h"
#include "eoc_comm/utility/dns_server.h"
int StartEocCommon() {
    dns_server_start();  /*dns服务*/
    if(g_eoc_config_init()<0){
        printf("g_eoc_config_init err\n");
        return -1;
    }
    int eoc_port = 6526;
    string eoc_host = "ehctest.eoc.aipark.com";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    string ipaddr="111.62.28.98";
    int ret = db_parking_lot_get_cloud_addr_from_factory(eoc_host, &eoc_port, file_host, &file_port);
    if(ret==0){
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n",eoc_host.c_str(),eoc_port);
    }
    eoc_port = 6526;
    eoc_host = "ehctest.eoc.aipark.com";
//    url_get(eoc_host,ipaddr);
    eoc_communication_start(ipaddr.c_str(), 6526);
}
