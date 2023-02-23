//
// Created by lining on 2023/2/23.
//

#include "DBCom.h"
#include "sqliteApi.h"
#include <glog/logging.h>

#include "DBTable.h"

DB_Base_Set_T g_Base_Set;
DB_Intersection_t g_Intersection;
DB_Fusion_Para_Set_T g_Fusion_Para_Set;
std::vector<DB_Associated_Equip_T> g_Associated_Equip;


int g_Base_Set_init(void) {
    int ret = 0;
    std::string db_data_version = "";
    getVersion(db_data_version);
    LOG(INFO) << "version:" << db_data_version;

    ret = get_base_set(g_Base_Set);
    if (0 == ret) {
        LOG(INFO) << "get g_Base_Set success";
    }

    LOG(INFO) << "g_Base_Set city=" << g_Base_Set.City;

    return 0;
}

int g_Intersection_init(void) {
    int ret = 0;
    ret = get_belong_intersection(g_Intersection);
    if (0 == ret) {
        LOG(INFO) << "get intersection success";
    }

    LOG(INFO) << "g_Intersection name=" << g_Intersection.Name;

    return 0;
}

int g_Fusion_Para_Set_init(void) {
    int ret = 0;
    ret = get_fusion_para_set(g_Fusion_Para_Set);
    if (0 == ret) {
        LOG(INFO) << "get g_Fusion_Para_Set success";
    }

    return 0;
}

int g_Associated_Equip_init(void) {
    int ret = 0;
    ret = get_associated_equip(g_Associated_Equip);
    if (0 == ret) {
        LOG(INFO) << "get g_Associated_Equip success";
    }

    return 0;
}

int g_config_init(void) {
    int ret = 0;
    //1.初始化数据库表
    if (tableInit(roadeside_parking_db.path, roadeside_parking_db.version) != 0) {
        LOG(ERROR) << "eoc db table init err";
        return -1;
    }
    //初始化全局变量
    g_Base_Set_init();
    g_Intersection_init();
    g_Fusion_Para_Set_init();
    g_Associated_Equip_init();

    std::string version;
    getVersion(version);
    if (!version.empty()) {
        LOG(ERROR) << "get version:" << version;
        return 1;
    }
    LOG(INFO) << "eoc version null";
    return 0;
}

int
get_eoc_server_addr(std::string &server_path, int &server_port, std::string &file_server_path, int &file_server_port) {
    server_port = 0;
    file_server_port = 0;
    db_parking_lot_get_cloud_addr_from_factory(server_path, server_port, file_server_path, file_server_port);
    return 0;
}
