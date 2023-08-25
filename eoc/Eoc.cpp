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
