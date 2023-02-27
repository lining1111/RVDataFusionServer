//
// Created by lining on 2023/2/23.
//

#include "DBCom.h"
#include "sqliteApi.h"
#include <glog/logging.h>

#include "DBTable.h"

DBBaseSet g_BaseSet;
DBIntersection g_Intersection;
DBFusionParaSet g_FusionParaSet;
std::vector<DBAssociatedEquip> g_AssociatedEquips;


int g_BaseSetInit(void) {
    int ret = 0;
    std::string db_data_version = "";
    DBDataVersion dbDataVersion;
    dbDataVersion.selectFromDB();
    db_data_version = dbDataVersion.version;
    LOG(INFO) << "version:" << db_data_version;

    ret = g_BaseSet.selectFromDB();
    if (0 == ret) {
        LOG(INFO) << "get g_BaseSet success";
    }

    LOG(INFO) << "g_BaseSet city=" << g_BaseSet.City;

    return 0;
}

int g_IntersectionInit(void) {
    int ret = 0;
    ret = g_Intersection.selectFromDB();
    if (0 == ret) {
        LOG(INFO) << "get intersection success";
    }

    LOG(INFO) << "g_Intersection name=" << g_Intersection.Name;

    return 0;
}

int g_FusionParaSetInit(void) {
    int ret = 0;
    ret = g_FusionParaSet.selectFromDB();
    if (0 == ret) {
        LOG(INFO) << "get g_FusionParaSet success";
    }

    return 0;
}

int g_AssociatedEquipsInit(void) {
    int ret = 0;
    ret = getAssociatedEquips(g_AssociatedEquips);
    if (0 == ret) {
        LOG(INFO) << "get g_AssociatedEquips success";
    }

    return 0;
}

int globalConfigInit(void) {
    int ret = 0;

    //1.初始化数据库表
    if (tableInit(eoc_configure.path, eoc_configure.version) != 0) {
        LOG(ERROR) << "eoc db table init err";
        return -1;
    }
    //初始化全局变量
    g_BaseSetInit();
    g_IntersectionInit();
    g_FusionParaSetInit();
    g_AssociatedEquipsInit();

    std::string version;
    DBDataVersion dbDataVersion;
    dbDataVersion.selectFromDB();
    version = dbDataVersion.version;
    if (!version.empty()) {
        LOG(ERROR) << "get version:" << version;
        return 1;
    }
    LOG(INFO) << "eoc version null";
    return 0;
}

int getEOCInfo(std::string &server_path, int &server_port, std::string &file_server_path, int &file_server_port) {
    server_port = 0;
    file_server_port = 0;
    dbGetCloudInfo(server_path, server_port, file_server_path, file_server_port);
    return 0;
}
