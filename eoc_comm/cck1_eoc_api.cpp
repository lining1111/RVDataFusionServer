
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ifaddrs.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>


#include "cJSON.h"
#include "ttcp.hpp"
#include "cck1_eoc_api.hpp"
#include "sqlite_eoc_db.h"
#include "configure_eoc_init.h"
#include "base64.h"
#include "dns_server.h"
#include "logger.h"
#include "md5.h"

using namespace std;


//mcu通信
//    if(mcu_dev_init() > 0)
//    {
//        start_c500_comm();
//    }


int start_k1_comm()
{
    if(mcu_dev_init() > 0)
    {
        start_c500_comm();
    }
    return 0;
}

static int get_json_int(cJSON *j_data, string str)
{
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get int %s err", str.c_str());
        return 0;
    }
    else
    {
        return cJSON_GetObjectItem(j_data, str.c_str())->valueint;
    }
}
static double get_json_double(cJSON *j_data, string str)
{
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get int %s err", str.c_str());
        return 0;
    }
    else
    {
        return cJSON_GetObjectItem(j_data, str.c_str())->valuedouble;
    }
}
static string get_json_string(cJSON *j_data, string str)
{
    string get_str;
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get string %s err", str.c_str());
        get_str = "";
        return get_str;
    }
    else
    {
        if(cJSON_GetObjectItem(j_data, str.c_str())->valuestring != NULL)
        {
            get_str = cJSON_GetObjectItem(j_data, str.c_str())->valuestring;
        }
        else
        {
            DBG("%s = null", str.c_str());
            get_str = "";
        }
        return get_str;
    }
}
/*
 * 函数功能：产生uuid
 * 参数：无
 * 返回值：uuid的string
 * */
static string random_uuid()
{
    char buf[37] = {0};
    struct timeval tmp;
    const char *c = "89ab";
    char *p = buf;
    unsigned int n,b;
    gettimeofday(&tmp, NULL);
    srand(tmp.tv_usec);

    for(n = 0; n < 16; ++n){
        b = rand()%65536;
        switch(n){
            case 6:
                sprintf(p, "4%x", b%15);
                break;
            case 8:
                sprintf(p, "%c%x", c[rand()%strlen(c)], b%15);
                break;
            default:
                sprintf(p, "%02x", b);
                break;
        }
        p += 2;
        switch(n){
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    *p = 0;
    return string(buf);
}

static int compute_file_md5(const char *file_path, const char *md5_check)
{
    int fd;
    int ret;
    unsigned char data[1024];
    unsigned char md5_value[16];
    char md5_tmp[64] = {0};
    MD5_CTX md5;

    fd = open(file_path, O_RDONLY);
    if (-1 == fd){
        ERR("%s:open file[%s] Err", __FUNCTION__, file_path);
        return -1;
    }
    MD5Init(&md5);
    while (1){
        ret = read(fd, data, 1024);
        if (-1 == ret){
            ERR("%s:read file[%s] Err", __FUNCTION__, file_path);
            return -1;
        }
        MD5Update(&md5, data, ret);
        if(0 == ret || ret < 1024){
            break;
        }
    }
    close(fd);
    MD5Final(&md5, md5_value);

    for(int i=0; i<16; i++){
        snprintf(md5_tmp + i*2, 2+1, "%02x", md5_value[i]);
    }
    
    if(strcmp(md5_check, md5_tmp) == 0){
        return 0;
    }else{
        INFO("%s md5check failed:file_md5=%s,check_md5=%s", __FUNCTION__, md5_tmp, md5_check);
    }

    return -1;
}


/*
 * 状态上传
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks001_status_send(tcp_client_t* client, const char *comm_guid)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包发送信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    if(strlen(comm_guid) > 0)
    {
        cJSON_AddStringToObject(root, "Guid", comm_guid);
    }
    else
    {
        cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    }
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S001);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S001);
    cJSON *state_array_json = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "CameraControllerStates", state_array_json);
    
    std::vector<MCU_DEV_INFO_STRU> c500_stats;
    get_mcu_dev_status_info(c500_stats);

    for(size_t i=0; i<c500_stats.size(); i++)
    {
        cJSON *control_obj = NULL;
        int camera_state = 0;   //0,离线  ；1，在线
        if(c500_stats[i].status <= 0)
        {
            camera_state = 0;
        }
        else
        {
            camera_state = 1;
        }
        char control_version[16] = {0};
        sprintf(control_version, "%d", c500_stats[i].data.version);
        cJSON_AddItemToArray(state_array_json,control_obj=cJSON_CreateObject());
        cJSON_AddStringToObject(control_obj, "CameraControllerGuid", c500_stats[i].camera_controller_guid);
        cJSON_AddStringToObject(control_obj, "SnNum", c500_stats[i].sn_num);
        cJSON_AddNumberToObject(control_obj, "State", camera_state);
        cJSON_AddNumberToObject(control_obj, "Voltage", c500_stats[i].data.voltage);            //电压值 12±10V
        cJSON_AddNumberToObject(control_obj, "Humidity", c500_stats[i].data.humidity);      //湿度值 0-100
        cJSON_AddNumberToObject(control_obj, "Temperature", c500_stats[i].data.temperature);    //温度值 -40~40
        cJSON_AddNumberToObject(control_obj, "AngleX", c500_stats[i].data.yaw);     //水平
        cJSON_AddNumberToObject(control_obj, "AngleY", c500_stats[i].data.pitch);       //俯仰
        cJSON_AddNumberToObject(control_obj, "AngleZ", c500_stats[i].data.roll);        //翻滚
        cJSON_AddNumberToObject(control_obj, "AngleState", c500_stats[i].data.angle_warn? 0:1); //角度报警 0报警；1正常
        cJSON_AddNumberToObject(control_obj, "QuakeState", c500_stats[i].data.motion_warn? 0:1);    //震动报警 0报警；1正常
        cJSON_AddNumberToObject(control_obj, "Fan1State", c500_stats[i].data.fan1_state);       //风扇1状态 0关；1开
        cJSON_AddNumberToObject(control_obj, "Fan2State", c500_stats[i].data.fan2_state);       //风扇2状态 0关；1开
//        cJSON_AddNumberToObject(control_obj, "LedLightness", c500_stats[i].data.led_lightness);//补光灯亮度值 5-75
//        cJSON_AddNumberToObject(control_obj, "LedOnLineState", c500_stats[i].data.led_online_state);    //补光灯链路状态 0断开；1连接
//        cJSON_AddNumberToObject(control_obj, "LedState", c500_stats[i].data.led_state);     //补光灯开关 0关；1开
        cJSON_AddNumberToObject(control_obj, "HeaterState", c500_stats[i].data.heat_state); //加热器开关 0关；1开
        cJSON_AddNumberToObject(control_obj, "RelayState", 1);  //继电器状态 0下电；1上电
        cJSON_AddStringToObject(control_obj, "FirmwareVersion", control_version);   //固件版本
        cJSON_AddNumberToObject(control_obj, "GyroscopeChipState", c500_stats[i].data.gryo_inited); //陀螺仪芯片状态 0异常；1正常
        cJSON_AddNumberToObject(control_obj, "Fan1AlarmState", c500_stats[i].data.fan1_fault_state? 0:1); //风扇1报警状态 0报警；1正常
        cJSON_AddNumberToObject(control_obj, "Fan2AlarmState", c500_stats[i].data.fan2_fault_state? 0:1); //风扇2报警状态 0报警；1正常
        cJSON_AddNumberToObject(control_obj, "AngleThresholdValueX", c500_stats[i].data.angle_threshold_valueX);    //姿态角度方位，角度报警阈值
        cJSON_AddNumberToObject(control_obj, "AngleThresholdValueY", c500_stats[i].data.angle_threshold_valueY);    //姿态角度俯仰，角度报警阈值
        cJSON_AddNumberToObject(control_obj, "AngleThresholdValueZ", c500_stats[i].data.angle_threshold_valueZ);    //姿态角度翻滚，角度报警阈值
        cJSON_AddNumberToObject(control_obj, "AngleThresholdValueQuake", c500_stats[i].data.angle_threshold_value_quake);   //震动，角度报警阈值

        cJSON *led_info_obj = NULL;
        cJSON_AddItemToObject(control_obj, "LedInfo", led_info_obj=cJSON_CreateObject());
        char led_model[4] = {0};
        snprintf(led_model, 4, "%d", c500_stats[i].data.led_data.led_type);
        cJSON_AddStringToObject(led_info_obj, "LedModel", led_model);
        cJSON_AddNumberToObject(led_info_obj, "PhotosensitiveThresholdOn", c500_stats[i].data.led_data.photosensitive_on_value);
        cJSON_AddNumberToObject(led_info_obj, "PhotosensitiveThresholdUnder", c500_stats[i].data.led_data.photosensitive_off_value);

        cJSON *led_state_array = cJSON_CreateArray();
        cJSON_AddItemToObject(led_info_obj, "LedStateInfos", led_state_array);
        cJSON *led_obj = NULL;
    
        for(size_t led_i=0; led_i<c500_stats[i].data.led_data.led_state_data.size(); led_i++)
        {
            cJSON_AddItemToArray(led_state_array, led_obj=cJSON_CreateObject());
            cJSON_AddNumberToObject(led_obj, "LedIndex", c500_stats[i].data.led_data.led_state_data[led_i].led_index);  //第几路补光灯
            cJSON_AddNumberToObject(led_obj, "LedLightness", c500_stats[i].data.led_data.led_state_data[led_i].led_lightness);//补光灯亮度值 5-75
            cJSON_AddNumberToObject(led_obj, "LedOnLineState", c500_stats[i].data.led_data.led_online_state);    //补光灯链路状态 0断开；1连接
            cJSON_AddNumberToObject(led_obj, "LedState", c500_stats[i].data.led_data.led_state_data[led_i].led_state);     //补光灯开关 0关；1开
        }
    }
    
    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";
    //DBG("send_str:%s",(char*)send_str.c_str());
    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr001_status_deal(cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    int state = get_json_int(data_json,"State");

    if(state == 1) {
        DBG("矩阵控制器状态上传成功");
    } else {
        ERR("矩阵控制器状态上传失败, msg: %s", get_json_string(data_json,"Message").c_str());
    }

    return 0;
}
/*
 * 矩阵控制器状态获取
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int ckr002_server_get_status_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    ret = cks001_status_send(client, comm_guid);
    return ret;
}
/*
 * 矩阵控制器风扇控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks003_fan_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S003);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S003);
    cJSON_AddStringToObject(data, "CameraControllerGuid", cameraControllerGuid);
    cJSON_AddNumberToObject(data, "Command", command);
    cJSON_AddNumberToObject(data, "State", state);          //控制结果 0失败          1成功
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr003_fan_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    int Command = 0;    //控制类型 0关闭风扇；1开启风扇
    string ControllerGuid;
    string ControllerIP;
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    Command = get_json_int(data, "Command");
    ControllerGuid = get_json_string(data, "CameraControllerGuid");
    ret = get_controller_ip(ControllerGuid, ControllerIP);
    if(ret == -1)
    {
        DBG("receive guid err");
        ret = cks003_fan_control_send(client, comm_guid, ControllerGuid.c_str(),
                Command, 0, "receive guid err");
        return ret;
    }

    //控制风扇
    C500_CONTROL_STRU control_command;
    int control_result = -1;
    sprintf(control_command.control_code, "003");
    sprintf(control_command.camera_controller_guid, "%s", ControllerGuid.c_str());
    sprintf(control_command.ip, "%s", ControllerIP.c_str());
    control_command.comm_type = Command;
    control_command.comm_value[0] = 0;
    control_command.comm_value[1] = 0;
    control_command.comm_value[2] = 0;
    control_command.comm_value[3] = 0;
    set_controller_command(&control_command);
    int i = 0;
    while(1)
    {
        control_result = get_controller_process_result();
        if(control_result >= 0)
        {
            break;
        }
        if(i > 35)
        {
            DBG("sleep i=%d", i);
            control_result = 0;
            break;
        }
        i ++;
        sleep(1);
    }

    ret = cks003_fan_control_send(client, comm_guid, ControllerGuid.c_str(),
            Command, control_result, "finish");

    return ret;
}
/*
 * 矩阵控制器加热器控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks004_heat_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S004);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S004);
    cJSON_AddStringToObject(data, "CameraControllerGuid", cameraControllerGuid);
    cJSON_AddNumberToObject(data, "Command", command);
    cJSON_AddNumberToObject(data, "State", state);          //控制结果 0失败          1成功
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr004_heat_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    int Command = 0;    //控制类型 0关闭；1开启
    string ControllerGuid;
    string ControllerIP;
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    Command = get_json_int(data, "Command");
    ControllerGuid = get_json_string(data, "CameraControllerGuid");
    ret = get_controller_ip(ControllerGuid, ControllerIP);
    if(ret == -1)
    {
        DBG("receive guid err");
        ret = cks004_heat_control_send(client, comm_guid, ControllerGuid.c_str(),
                Command, 0, "receive guid err");
        return ret;
    }

    //控制风扇
    C500_CONTROL_STRU control_command;
    int control_result = -1;
    sprintf(control_command.control_code, "004");
    sprintf(control_command.camera_controller_guid, "%s", ControllerGuid.c_str());
    sprintf(control_command.ip, "%s", ControllerIP.c_str());
    control_command.comm_type = Command;
    control_command.comm_value[0] = 0;
    control_command.comm_value[1] = 0;
    control_command.comm_value[2] = 0;
    control_command.comm_value[3] = 0;
    set_controller_command(&control_command);
    int i = 0;
    while(1)
    {
        control_result = get_controller_process_result();
        if(control_result >= 0)
        {
            break;
        }
        if(i > 35)
        {
            DBG("sleep i=%d", i);
            control_result = 0;
            break;
        }
        i ++;
        sleep(1);
    }

    ret = cks004_heat_control_send(client, comm_guid, ControllerGuid.c_str(),
            Command, control_result, "finish");

    return ret;
}
/*
 * 矩阵控制器角度控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks005_angle_control_send(tcp_client_t* client, const char *comm_guid, C500_CONTROL_STRU &control_info, 
                                            int state, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S005);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S005);
    cJSON_AddStringToObject(data, "CameraControllerGuid", control_info.camera_controller_guid);
    cJSON_AddNumberToObject(data, "Command", control_info.comm_type);
    cJSON_AddNumberToObject(data, "ValueX", control_info.comm_value[3]);
    cJSON_AddNumberToObject(data, "ValueY", control_info.comm_value[2]);
    cJSON_AddNumberToObject(data, "ValueZ", control_info.comm_value[1]);
    cJSON_AddNumberToObject(data, "ValueQuake", control_info.comm_value[0]);
    cJSON_AddNumberToObject(data, "State", state);          //控制结果 0失败          1成功
    cJSON_AddStringToObject(data, "Message", msg);     

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr005_angle_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    int Command = 0;    //控制类型 1角度复位；2角度阈值设置
    string CameraControllerGuid;
    double quakeValue;  //震动阈值
    double yawValue;    //方位角度x
    double pitchValue;  //俯仰角度y
    double rollValue;   //翻滚角度z
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    Command = cJSON_GetObjectItem(data,"Command")->valueint;
    CameraControllerGuid = cJSON_GetObjectItem(data,"CameraControllerGuid")->valuestring;
    quakeValue = cJSON_GetObjectItem(data,"ValueQuake")->valuedouble;
    yawValue = cJSON_GetObjectItem(data,"ValueX")->valuedouble;
    pitchValue = cJSON_GetObjectItem(data,"ValueY")->valuedouble;
    rollValue = cJSON_GetObjectItem(data,"ValueZ")->valuedouble;
    //控制角度
    C500_CONTROL_STRU control_command;
    int control_result = -1;
    sprintf(control_command.control_code, "005");
    sprintf(control_command.camera_controller_guid, "%s", CameraControllerGuid.c_str());
    control_command.comm_type = Command;
    control_command.comm_value[0] = quakeValue;
    control_command.comm_value[1] = rollValue;
    control_command.comm_value[2] = pitchValue;
    control_command.comm_value[3] = yawValue;
    string cameraControllerIP;
    ret = get_controller_ip(CameraControllerGuid, cameraControllerIP);
    if(ret == -1)
    {
        DBG("receive guid err");
        ret = cks005_angle_control_send(client, comm_guid, control_command, 0, "receive guid err");
        return ret;
    }
    sprintf(control_command.ip, "%s", cameraControllerIP.c_str());
    
    set_controller_command(&control_command);
    int i = 0;
    while(1)
    {
        control_result = get_controller_process_result();
        if(control_result >= 0)
        {
            break;
        }
        if(i > 35)
        {
            DBG("sleep i=%d", i);
            control_result = 0;
            break;
        }
        i ++;
        sleep(1);
    }

    ret = cks005_angle_control_send(client, comm_guid, control_command, control_result, "finish");

    return ret;
}
/*
 * 矩阵控制器补光灯控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks006_led_control_send(tcp_client_t* client, const char *comm_guid, C500_CONTROL_STRU &control_info, 
                                            char *contr_str, int state)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S006);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S006);
    cJSON_AddStringToObject(data, "CameraControllerGuid", control_info.camera_controller_guid);
    cJSON_AddNumberToObject(data, "PhotosensitiveThresholdOn", control_info.comm_value[4]);
    cJSON_AddNumberToObject(data, "PhotosensitiveThresholdUnder", control_info.comm_value[5]);
    if(contr_str == NULL)
    {
        cJSON_AddNullToObject(data, "ControlInfos");
    }
    else
    {
        cJSON_AddItemToObject(data, "ControlInfos", cJSON_Parse(contr_str)); //字符串解析成json结构体
    }
    cJSON_AddNumberToObject(data, "State", state);          //控制结果 0失败 1成功
    cJSON_AddStringToObject(data, "Message", control_info.comm_msg); 

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr006_led_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    int Command = 0;    //控制类型 0关闭补光灯；1打开补光灯；2调节补光灯亮度
    string CameraControllerGuid;
    string cameraControllerIP;
    double lightValue = 0;
    cJSON *data = cJSON_GetObjectItem(json, "Data");
    int led_index = 0;
    int led_up_on = 0;
    int led_down_off = 0;
    int i = 0;
    
//    Command = cJSON_GetObjectItem(data,"Command")->valueint;
//    lightValue = cJSON_GetObjectItem(data,"Value")->valuedouble;
    CameraControllerGuid = cJSON_GetObjectItem(data,"CameraControllerGuid")->valuestring;
    if(cJSON_GetObjectItem(data,"PhotosensitiveThresholdOn") != NULL){
        led_up_on = cJSON_GetObjectItem(data,"PhotosensitiveThresholdOn")->valueint;
    }else{
        led_up_on = 0;
    }
    if(cJSON_GetObjectItem(data,"PhotosensitiveThresholdUnder") != NULL){
        led_down_off = cJSON_GetObjectItem(data,"PhotosensitiveThresholdUnder")->valueint;
    }else{
        led_down_off = 0;
    }
    //控制补光灯、光敏阈值
    C500_CONTROL_STRU control_command;
    int control_result = -1;
    sprintf(control_command.control_code, "006");
    sprintf(control_command.camera_controller_guid, "%s", CameraControllerGuid.c_str());
    control_command.comm_type = 2;
    control_command.comm_value[0] = lightValue;
    control_command.comm_add = 2;
    control_command.comm_value[4] = led_up_on;      //3880~4050
    control_command.comm_value[5] = led_down_off;   //3500-3850
    control_command.comm_value[6] = 0;
    control_command.comm_value[7] = 0;
    cJSON *control_infos = cJSON_GetObjectItem(data,"ControlInfos");
    if(control_infos == NULL)
    {
        DBG("receive ControlInfos null");
    }
    else
    {
        int array_size = cJSON_GetArraySize(control_infos);
        for(int array_i=0; array_i<array_size; array_i++){
            cJSON * CameraArrayArray = cJSON_GetArrayItem(control_infos, array_i);
            led_index = cJSON_GetObjectItem(CameraArrayArray,"LedIndex")->valueint;
        //    Command = cJSON_GetObjectItem(CameraArrayArray,"Command")->valueint;
            lightValue = cJSON_GetObjectItem(CameraArrayArray,"Value")->valuedouble;
            if(lightValue <= 0)
                lightValue = 0;
        //    control_command.comm_type = Command;
            control_command.comm_value[led_index - 1] = lightValue;
            if(array_i > 2)
                break;
        }
    }
    char *contr_info_str=NULL;
    if(control_infos != NULL)
    {
        contr_info_str=cJSON_PrintUnformatted(control_infos);//json结构体转换成字符串
    }
    ret = get_controller_ip(CameraControllerGuid, cameraControllerIP);
    if(ret == -1)
    {
        DBG("receive guid err");
        sprintf(control_command.comm_msg, "receive guid err");
        ret = cks006_led_control_send(client, comm_guid, control_command, contr_info_str, 0);
        if(contr_info_str)
        {
            free(contr_info_str);
        }
        return ret;
    }
    sprintf(control_command.ip, "%s", cameraControllerIP.c_str());

    set_controller_command(&control_command);
    i = 0;
    while(1)
    {
        control_result = get_controller_process_result_msg(control_command.comm_msg);   // 0失败,1成功
        if(control_result >= 0)
        {
            break;
        }
        if(i > 45)
        {
            DBG("sleep i=%d", i);
            control_result = 0;
            break;
        }
        i ++;
        sleep(1);
    }

    ret = cks006_led_control_send(client, comm_guid, control_command, contr_info_str, control_result);
    if(contr_info_str)
    {
        free(contr_info_str);
    }
    return ret;
}
/*
 * 矩阵控制器继电器控制命令
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int cks007_relay_control_send(tcp_client_t* client, const char *comm_guid, const char *cameraControllerGuid, 
                                            int command, int state, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S007);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S007);
    cJSON_AddStringToObject(data, "CameraControllerGuid", cameraControllerGuid);
    cJSON_AddNumberToObject(data, "Command", command);
    cJSON_AddNumberToObject(data, "State", state);          //控制结果 0失败          1成功
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
int ckr007_relay_control_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    int Command = 0;    //控制类型 0下电；1上电；2重启
    string ControllerGuid;
    string ControllerIP;
    cJSON *data = cJSON_GetObjectItem(json, "Data");

    Command = get_json_int(data, "Command");
    ControllerGuid = get_json_string(data, "CameraControllerGuid");
    ret = get_controller_ip(ControllerGuid, ControllerIP);
    if(ret == -1)
    {
        DBG("receive guid err");
        ret = cks007_relay_control_send(client, comm_guid, ControllerGuid.c_str(),
                Command, 0, "receive guid err");
        return ret;
    }

    //控制继电器
    C500_CONTROL_STRU control_command;
    int control_result = -1;
    sprintf(control_command.control_code, "007");
    sprintf(control_command.camera_controller_guid, "%s", ControllerGuid.c_str());
    sprintf(control_command.ip, "%s", ControllerIP.c_str());
    control_command.comm_type = Command;
    control_command.comm_value[0] = 0;
    control_command.comm_value[1] = 0;
    control_command.comm_value[2] = 0;
    control_command.comm_value[3] = 0;
    set_controller_command(&control_command);
    int i = 0;
    while(1)
    {
        control_result = get_controller_process_result();
        if(control_result >= 0)
        {
            break;
        }
        if(i > 35)
        {
            DBG("sleep i=%d", i);
            control_result = 0;
            break;
        }
        i ++;
        sleep(1);
    }

    ret = cks007_relay_control_send(client, comm_guid, ControllerGuid.c_str(),
            Command, control_result, "finish");

    return ret;
}


int cks008_download_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S008);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S008);
    cJSON_AddStringToObject(data, "CameraControllerGuid", guid);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 处理矩阵控制器升级的下载文件指令，将会把文件下载到指定目录下
 * */
int ckr008_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;

    cJSON *data = cJSON_GetObjectItem(json, "Data");

    EocDownloadsMsg rcv_download_msg;
    EocUpgradeDev rcv_controller_msg;
    rcv_download_msg.download_url = cJSON_GetObjectItem(data, "DownloadPath")->valuestring;
    rcv_download_msg.download_file_name = cJSON_GetObjectItem(data, "Filename")->valuestring;
    rcv_download_msg.download_file_size = atoi(cJSON_GetObjectItem(data, "FileSize")->valuestring);
    rcv_download_msg.download_file_md5 = cJSON_GetObjectItem(data, "FileMD5")->valuestring;
    rcv_controller_msg.dev_ip = cJSON_GetObjectItem(data, "CameraControllerIP")->valuestring;
    rcv_controller_msg.dev_guid = cJSON_GetObjectItem(data, "CameraControllerGuid")->valuestring;
    rcv_controller_msg.dev_type = EOC_UPGRADE_CONTROLLER;
    rcv_controller_msg.comm_guid = std::string(comm_guid);
//    rcv_download_msg.comm_guid = comm_guid;

    ret = cks008_download_send(client, comm_guid, rcv_controller_msg.dev_guid.c_str(), 
                            1, 1, 0, "Receive controller download msg succeed");
    if(ret != 0)return ret;
    //判断文件是否存在，已存在直接返回下载成功
    char update_file_name[256] = {0};
    snprintf(update_file_name, 256, "%s_%s", rcv_download_msg.download_file_md5.c_str(),
                rcv_download_msg.download_file_name.c_str());
    if(access(update_file_name, R_OK|R_OK) == 0)
    {
        DBG("%s:%s 文件已存在", __FUNCTION__, update_file_name);
        if(compute_file_md5(update_file_name,  rcv_download_msg.download_file_md5.c_str()) == 0)
        {
            DBG("下载文件MD5校验通过");
            return cks008_download_send(client, comm_guid, rcv_controller_msg.dev_guid.c_str(), 
                                                    2, 1, 0, "下载完成 MD5 OK");
        }
    }    
    //添加下载任务
    ret = add_eoc_download_event(rcv_download_msg, rcv_controller_msg);
    if(ret < 0)
    {
        DBG("添加下载任务失败");
        return cks008_download_send(client, comm_guid, rcv_controller_msg.dev_guid.c_str(), 
                                                    2, 0, 0, "add download_event Err");
    }
    else if(1 == ret)
    {
        DBG("下载任务存在，返回错误");
        return cks008_download_send(client, comm_guid, rcv_controller_msg.dev_guid.c_str(), 
                                                    2, 0, 0, "该下载正在进行");
    }
    
    return 0;
}

int cks009_upgrade_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_CONTROLLER_S009);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_CONTROLLER_S009);
    cJSON_AddStringToObject(data, "CameraControllerGuid", guid);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

int ckr009_upgrade_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;
    char update_file_name[128] = {0};
    EocUpgradeDev rcv_data;

    cJSON *data = cJSON_GetObjectItem(json, "Data");
    snprintf(update_file_name, 128, "%s_%s", 
            cJSON_GetObjectItem(data, "FileMD5")->valuestring, 
            cJSON_GetObjectItem(data, "Filename")->valuestring);
    rcv_data.dev_guid = cJSON_GetObjectItem(data, "CameraControllerGuid")->valuestring;
    rcv_data.dev_ip = cJSON_GetObjectItem(data, "CameraControllerIP")->valuestring;
    rcv_data.sw_version = cJSON_GetObjectItem(data, "FileVersion")->valuestring;
    rcv_data.hw_version = cJSON_GetObjectItem(data, "CurrentFirmwareVersion")->valuestring;

    cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 1, 1, 0, "接收到升级命令");
    
    //1 判断文件是否存在
    if(access(update_file_name, R_OK|R_OK) != 0){
        ERR("%s:%s 文件不存在", __FUNCTION__, update_file_name);
        return cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "文件不存在");
    }
    //2 MD5校验
    if(compute_file_md5(update_file_name,  cJSON_GetObjectItem(data, "FileMD5")->valuestring) != 0){
        ERR("%s:%s MD5校验失败", __FUNCTION__, update_file_name);
        return cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "MD5校验失败");
    }
    ret = cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 1, 1, 10, "校验完成开始升级");
    add_c500_upgrade_task(rcv_data.dev_ip.c_str(), update_file_name, rcv_data.hw_version.c_str(), rcv_data.sw_version.c_str());
    rcv_data.dev_type = EOC_UPGRADE_CONTROLLER;
    rcv_data.comm_guid = comm_guid;
    rcv_data.dev_model = "";
    ret = add_eoc_upgrade_event(rcv_data);
    if(ret < 0)
    {
        ret = cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "添加升级任务失败");
    }
    else if(1 == ret)
    {
        ret = cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "该升级任务已存在");
    }
    else if(0 == ret)
    {
        if(rcv_data.dev_ip.length() < 7)
        {
            ret = get_controller_ip(rcv_data.dev_guid, rcv_data.dev_ip);
            if(0 > ret)
            {
                ret = cks009_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "矩阵控制器guid错误");
                return ret;
            }
        }
        add_c500_upgrade_task(rcv_data.dev_ip.c_str(), update_file_name, rcv_data.hw_version.c_str(), rcv_data.sw_version.c_str());
    }

    return ret;
}
//向EOC返回矩阵控制器升级状态 返回值：<0发送错误，0升级中发送状态完成，1升级结束
int send_ck1_upgrade_state(tcp_client_t* client, EocUpgradeDev *upgrade_msg)
{
    int ret = 0;
    //取矩阵控制器的升级进度返回
    bool upgrade_ret = 0;
    C500_UPGRADE_STATE state;
    int step_total;
    int current_step;
    int progress;
    char message[64] = {0};
    std::string ck1_ip;
    get_controller_ip(upgrade_msg->dev_guid, ck1_ip);
    upgrade_ret = get_c500_upgrade_state(ck1_ip.c_str(), &state,
            &step_total, &current_step, &progress, message);
    DBG("%s ret=%d,state=%d,total=%d,step=%d,progress=%d,message=%s",
            ck1_ip.c_str(), upgrade_ret, state, step_total, current_step, progress, message);
//ceshi    state = C500_UPGRADE_SUCCEED;
    if(state == C500_UPGRADE_WAIT)
    {

    }
    else if(state == C500_UPGRADE_DOING)
    {
        ret = cks009_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(), 
                1, 1, progress, message);

    }
    else if(state == C500_UPGRADE_SUCCEED)
    {
        ret = cks009_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(), 
                2, 1, 0, message);
        return 1;
    }
    else if(state == C500_UPGRADE_FAILED)
    {
        ret = cks009_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(), 
                2, 0, progress, message);
        return 1;
    }

    if(time(NULL) >= upgrade_msg->start_upgrade_time + 9*60)
    {
        ret = cks009_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(), 
                2, 0, progress, "升级超时");
        return 1;
    }

    return 0;
}




