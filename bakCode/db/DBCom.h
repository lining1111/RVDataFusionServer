//
// Created by lining on 2023/2/23.
//

#ifndef DBCOM_H
#define DBCOM_H

#include <string>


#include "DBTable.h"
/**
 * 数据库通信类
 *
 */

extern DB_Base_Set_T g_Base_Set;
extern DB_Intersection_t g_Intersection;
extern DB_Fusion_Para_Set_T g_Fusion_Para_Set;
extern std::vector<DB_Associated_Equip_T> g_Associated_Equip;

/**
 * 初始化全局变量 g_base_set
 * @return
 */
int g_Base_Set_init(void);

int g_Intersection_init(void);

int g_Fusion_Para_Set_init(void);

int g_Associated_Equip_init(void);

//返回值：1，取到eoc配置；0，eoc配置没下发；-1，初始化数据库失败
int g_config_init(void);
//从~/bin/RoadsideParking.db取eoc服务器地址
int get_eoc_server_addr(std::string &server_path, int& server_port, std::string &file_server_path, int& file_server_port);

#endif //DBCOM_H
