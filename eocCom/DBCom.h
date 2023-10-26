//
// Created by lining on 2023/2/23.
//

#ifndef DBCOM_H
#define DBCOM_H

#include <string>
#include <vector>
#include "db/eoc_configure.h"
using namespace eoc_configure;
/**
 * 数据库通信类
 *
 */

extern DBBaseSet g_BaseSet;
extern DBIntersection g_Intersection;
extern std::vector<DBAssociatedEquip> g_AssociatedEquips;

/**
 * 初始化全局变量 g_base_set
 * @return
 */
int g_BaseSetInit(void);

int g_IntersectionInit(void);

int g_AssociatedEquipsInit(void);

//返回值：1，取到eoc配置；0，eoc配置没下发；-1，初始化数据库失败
int globalConfigInit(void);
//从~/bin/RoadsideParking.db取eoc服务器地址
int getEOCInfo(std::string &server_path, int& server_port, std::string &file_server_path, int& file_server_port);

#endif //DBCOM_H
