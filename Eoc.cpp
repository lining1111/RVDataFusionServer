//
// Created by lining on 11/4/22.
//

#include <sqlite_eoc_db.h>
#include "yyls_eoc_comm.hpp"
#include "Eoc.h"
#include "configure_eoc_init.h"
#include "dns_server.h"
int StartEocCommon() {
    dns_server_start();  /*dns服务*/
    if(g_eoc_config_init()<0){
        printf("g_eoc_config_init err\n");
        return -1;
    }
    int eoc_port = 6524;
    string eoc_host = "ehctest.eoc.aipark.com";
    int file_port = 7000;
    string file_host = "ehctest.eoc.aipark.com";
    string ipaddr;
    int ret = db_parking_lot_get_cloud_addr_from_factory(eoc_host, &eoc_port, file_host, &file_port);
    if(ret==0){
        printf("db_parking_lot_get_cloud_addr_from_factory eoc_host:%s eoc_port:%d\n",eoc_host.c_str(),eoc_port);
    }
    eoc_port = 6524;
    eoc_host = "ehctest.eoc.aipark.com";
    url_get(eoc_host,ipaddr);
    eoc_communication_start(ipaddr.c_str(), 6524);
}
