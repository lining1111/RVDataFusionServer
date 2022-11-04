#ifndef __CCK1_EOC_API_HPP__
#define __CCK1_EOC_API_HPP__


#include <string>
#include <vector>
#include "c500SDK.h"
#include "yyls_eoc_comm.hpp"

#define EOC_CONTROLLER_S001 "ControllerS001"    //矩阵控制器状态监控
#define EOC_CONTROLLER_R001 "ControllerR001"
#define EOC_CONTROLLER_S002 "ControllerS002"    //矩阵控制器状态获取
#define EOC_CONTROLLER_R002 "ControllerR002"
#define EOC_CONTROLLER_S003 "ControllerS003"    //矩阵控制器风扇控制
#define EOC_CONTROLLER_R003 "ControllerR003"
#define EOC_CONTROLLER_S004 "ControllerS004"    //矩阵控制器加热器控制
#define EOC_CONTROLLER_R004 "ControllerR004"
#define EOC_CONTROLLER_S005 "ControllerS005"    //矩阵控制器角度控制
#define EOC_CONTROLLER_R005 "ControllerR005"
#define EOC_CONTROLLER_S006 "ControllerS006"    //矩阵控制器补光灯控制
#define EOC_CONTROLLER_R006 "ControllerR006"
#define EOC_CONTROLLER_S007 "ControllerS007"    //矩阵控制器继电器控制
#define EOC_CONTROLLER_R007 "ControllerR007"
#define EOC_CONTROLLER_S008 "ControllerS008"    //矩阵控制器软件下载推送
#define EOC_CONTROLLER_R008 "ControllerR008"
#define EOC_CONTROLLER_S009 "ControllerS009"    //矩阵控制器软件升级确认
#define EOC_CONTROLLER_R009 "ControllerR009"


int start_k1_comm();

/*
 * 状态上传
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks001_status_send(tcp_client_t* client, const char *comm_guid);
int ckr001_status_deal(cJSON *json);
/*
 * 矩阵控制器状态获取
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int ckr002_server_get_status_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);
/*
 * 矩阵控制器风扇控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks003_fan_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg);
int ckr003_fan_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);
/*
 * 矩阵控制器加热器控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks004_heat_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg);
int ckr004_heat_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);

/*
 * 矩阵控制器角度控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks005_angle_control_send(tcp_client_t* client, const char *comm_guid, C500_CONTROL_STRU &control_info, 
                                            int state, const char *msg);
int ckr005_angle_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);
/*
 * 矩阵控制器补光灯控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks006_led_control_send(tcp_client_t* client, const char *comm_guid, C500_CONTROL_STRU &control_info, 
                                            char *contr_str, int state);
int ckr006_led_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);
/*
 * 矩阵控制器继电器控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks007_relay_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg);
int ckr007_relay_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);


int cks008_download_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg);

int ckr008_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);
int cks009_upgrade_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg);
int ckr009_upgrade_deal(tcp_client_t* client, const char *comm_guid, cJSON *json);



//向EOC返回矩阵控制器升级状态 返回值：<0发送错误，0升级中发送状态完成，1升级结束
int send_ck1_upgrade_state(tcp_client_t* client, EocUpgradeDev *upgrade_msg);








#endif
