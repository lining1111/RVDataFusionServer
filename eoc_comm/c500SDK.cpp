#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <vector>

#ifndef __WIN32
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#include "md5.h"
#include "c500SDK.h"
#include "logger.h"
//#include "sqlite_db.h"
#include "configure_eoc_init.h"

typedef struct {
    volatile int  fd;
    char ip[64];
    int  iaddr;
    int  port;
    int  status;    // 未连接-1，启动中0，连接正常1
    volatile int  update_mode;
    monitor_data_t data;
} monitor_client_t;

typedef struct {
    char ip[64];
    volatile int  msg_fd;
    volatile int  msg_cli_fd;
    volatile int  conn_ok;
} heatsdk_client_t;

static pthread_mutex_t heatsdk_client_mutex = PTHREAD_MUTEX_INITIALIZER;
#define HEATSDK_MAX_CLIENT  128
static heatsdk_client_t g_c500_list[HEATSDK_MAX_CLIENT];
static monitor_callback_t local_cb = NULL;

class LockGuard
{
public:
	LockGuard(pthread_mutex_t *lock):
		lock_(lock)
	{
		pthread_mutex_lock(lock_);
	}
	virtual ~LockGuard()
	{
		pthread_mutex_unlock(lock_);
	}
private:
 	pthread_mutex_t *lock_;	
};

/*
**添加mcu上传矩阵信息
*/
static std::vector<MCU_DEV_INFO_STRU> mcu_dev_data;
pthread_mutex_t mcu_info_mutex = PTHREAD_MUTEX_INITIALIZER;

static C500_CONTROL_STRU g_c500_control_command;
pthread_mutex_t c500_control_mutex = PTHREAD_MUTEX_INITIALIZER;

static std::vector<C500_UPGRADE_STRU> g_c500_upgrade_task;	//升级任务
pthread_mutex_t c500_upgrade_mutex = PTHREAD_MUTEX_INITIALIZER;

extern EOC_Base_Set g_eoc_base_set;    //基础配置

void mcu_callback(const char *ip, monitor_data_t *data, int status)
{
    if (status == HEATSDK_DEV_BOOTING) {
        DBG("dev %s is booting, pls wait", ip);
    } else if (status == HEATSDK_DEV_UPGRADE) {
        DBG("dev %s is upgrading, pls wait", ip);
    } else if (status == HEATSDK_NET_ERROR) {
        DBG("dev %s is not connected", ip);
    } else if (status == HEATSDK_DEV_OK) {
        DBG("设备: %s 状态: ", ip);
        DBG("############翻滚角度: %f", data->roll); // roll
        DBG("############俯仰角度: %f", data->pitch); // pitch
        DBG("############方位角度: %f", data->yaw); // yaw
        DBG("############温度: %f", data->temperature);
        DBG("############湿度: %f", data->humidity);
        DBG("############电压: %f", data->voltage);
        DBG("############角度报警: %d 运动报警: %d", data->angle_warn,
               data->motion_warn);
        DBG("############加热状态: %d， 数据时间: %lld", data->heat_state,
               (long long int)data->data_timestamp);
        DBG("############陀螺仪状态：%d，初始状态：%d", data->gyrochip_inited, data->gryo_inited);
		DBG("############风扇1状态：%d，风扇2状态：%d", data->fan1_state, data->fan2_state);
		DBG("############补光灯开关状态：%d，485连接状态：%d", data->led_state, data->led_data.led_online_state);
    }
}

int mcu_dev_init(void)
{
	int ret = 0;

	mcu_dev_data.clear();
#if 0
	for(size_t i=0; i<g_camera_array.size(); i++)
	{
		for(size_t j=0; j<g_camera_array[i].cameraControllerInfo.size(); j++)
		{
			MCU_DEV_INFO_STRU mcu_dev_tmp;
			sprintf(mcu_dev_tmp.camera_controller_guid, "%s", g_camera_array[i].cameraControllerInfo[j].Guid);
			sprintf(mcu_dev_tmp.camera_controller_name, "%s", g_camera_array[i].cameraControllerInfo[j].Name);
			sprintf(mcu_dev_tmp.ip, "%s", g_camera_array[i].cameraControllerInfo[j].IP);
			mcu_dev_tmp.status = -1;
			mcu_dev_tmp.info_warn_chang = 0;
			memset((char*)&mcu_dev_tmp.data, 0, sizeof(monitor_data_t));
            memset(mcu_dev_tmp.sn_num, 0, sizeof(mcu_dev_tmp.sn_num));
			mcu_dev_data.push_back(mcu_dev_tmp);
		}
	}
#endif
    if(strlen(g_eoc_base_set.CCK1Guid) >= 7)
    {
        MCU_DEV_INFO_STRU mcu_dev_tmp;
        sprintf(mcu_dev_tmp.camera_controller_guid, "%s", g_eoc_base_set.CCK1Guid);
        sprintf(mcu_dev_tmp.camera_controller_name, "%s", g_eoc_base_set.CCK1Ip);
        string guid = g_eoc_base_set.CCK1Guid;
        string ip;
        get_controller_ip(guid, ip);
        sprintf(mcu_dev_tmp.ip, "%s", ip.c_str());
        mcu_dev_tmp.status = -1;
        mcu_dev_tmp.info_warn_chang = 0;
        memset((char*)&mcu_dev_tmp.data, 0, sizeof(monitor_data_t));
        memset(mcu_dev_tmp.sn_num, 0, sizeof(mcu_dev_tmp.sn_num));
        mcu_dev_data.push_back(mcu_dev_tmp);
    }

	return mcu_dev_data.size();
}

int start_c500_comm(void)
{
	int ret = 0;
	DBG("start mcu communication");
	memset(g_c500_control_command.ip, 0, 32);
	memset(g_c500_control_command.control_code, 0, 16);
	g_c500_control_command.comm_result = 0;
	for(size_t mcu_start=0; mcu_start<mcu_dev_data.size(); mcu_start++)
	{
	//	sleep(3);
		ret = heatsdk_client_start(mcu_dev_data[mcu_start].ip);
		DBG("start dev %s loop return: %d", mcu_dev_data[mcu_start].ip, ret);
	}

	return 0;
}

size_t get_mcu_device_size(void)
{
	return mcu_dev_data.size();
}
/*取矩阵控制器告警状态是否变化，返回值 1，告警状态改变；2，没改变*/
int get_mcu_warn_change_flag(void)
{
	size_t mcu_i = 0;
	size_t mcu_num = get_mcu_device_size();

	LockGuard lock_gurad(&mcu_info_mutex);
	for(mcu_i=0; mcu_i<mcu_num; mcu_i++)
	{
		if(mcu_dev_data[mcu_i].info_warn_chang == 1)
			break;
	}

	if(mcu_i < mcu_num)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/*矩阵控制器告警状态改变标志清0*/
int clean_mcu_warn_change_flag(void)
{
	size_t mcu_num = get_mcu_device_size();
	
	LockGuard lock_gurad(&mcu_info_mutex);
	for(size_t mcu_i=0; mcu_i<mcu_num; mcu_i++)
	{
		if(mcu_dev_data[mcu_i].info_warn_chang==1)
		{
			mcu_dev_data[mcu_i].info_warn_chang = 0;
		}
	}

	return 0;
}
/*取矩阵控制器状态信息*/
int get_mcu_dev_status_info(std::vector<MCU_DEV_INFO_STRU> &data)
{
	size_t mcu_num = get_mcu_device_size();

	LockGuard lock_gurad(&mcu_info_mutex);
	for(size_t i=0; i<mcu_num; i++)
	{
		MCU_DEV_INFO_STRU mcu_info_data;

		strcpy(mcu_info_data.camera_controller_guid, mcu_dev_data[i].camera_controller_guid);
		strcpy(mcu_info_data.camera_controller_name, mcu_dev_data[i].camera_controller_name);
		strcpy(mcu_info_data.ip, mcu_dev_data[i].ip);
		mcu_info_data.status = mcu_dev_data[i].status;
		mcu_info_data.info_warn_chang = mcu_dev_data[i].info_warn_chang;
		mcu_info_data.data.roll = mcu_dev_data[i].data.roll;
		mcu_info_data.data.pitch = mcu_dev_data[i].data.pitch;
		mcu_info_data.data.yaw = mcu_dev_data[i].data.yaw;
		mcu_info_data.data.temperature = mcu_dev_data[i].data.temperature;
		mcu_info_data.data.humidity = mcu_dev_data[i].data.humidity;
		mcu_info_data.data.voltage = mcu_dev_data[i].data.voltage;
		mcu_info_data.data.angle_warn = mcu_dev_data[i].data.angle_warn;
		mcu_info_data.data.motion_warn = mcu_dev_data[i].data.motion_warn;
		mcu_info_data.data.gyrochip_inited = mcu_dev_data[i].data.gyrochip_inited;
		mcu_info_data.data.gryo_inited = mcu_dev_data[i].data.gryo_inited;
		mcu_info_data.data.heat_state = mcu_dev_data[i].data.heat_state;
		mcu_info_data.data.fan1_state = mcu_dev_data[i].data.fan1_state;
		mcu_info_data.data.fan2_state = mcu_dev_data[i].data.fan2_state;
		mcu_info_data.data.led_state = mcu_dev_data[i].data.led_state;
		mcu_info_data.data.led_online_state = mcu_dev_data[i].data.led_online_state;
//		mcu_info_data.data.led_lightness = mcu_dev_data[i].data.led_lightness;
		mcu_info_data.data.data_timestamp = mcu_dev_data[i].data.data_timestamp;
		mcu_info_data.data.version = mcu_dev_data[i].data.version;

		mcu_info_data.data.fan1_fault_state = mcu_dev_data[i].data.fan1_fault_state;
		mcu_info_data.data.fan2_fault_state = mcu_dev_data[i].data.fan2_fault_state;
		mcu_info_data.data.angle_threshold_valueX = mcu_dev_data[i].data.angle_threshold_valueX;
		mcu_info_data.data.angle_threshold_valueY = mcu_dev_data[i].data.angle_threshold_valueY;
		mcu_info_data.data.angle_threshold_valueZ = mcu_dev_data[i].data.angle_threshold_valueZ;
		mcu_info_data.data.angle_threshold_value_quake = mcu_dev_data[i].data.angle_threshold_value_quake;

		mcu_info_data.data.led_data.led_online_state = mcu_dev_data[i].data.led_data.led_online_state;
		mcu_info_data.data.led_data.led_type = mcu_dev_data[i].data.led_data.led_type;
		mcu_info_data.data.led_data.photosensitive_on_value = mcu_dev_data[i].data.led_data.photosensitive_on_value;
		mcu_info_data.data.led_data.photosensitive_off_value = mcu_dev_data[i].data.led_data.photosensitive_off_value;
		for(size_t led_i=0; led_i<mcu_dev_data[i].data.led_data.led_state_data.size(); led_i++)
		{
			led_state_t led_state;
			led_state.led_index = mcu_dev_data[i].data.led_data.led_state_data[led_i].led_index;
			led_state.led_lightness = mcu_dev_data[i].data.led_data.led_state_data[led_i].led_lightness;
			led_state.led_state = mcu_dev_data[i].data.led_data.led_state_data[led_i].led_state;
			mcu_info_data.data.led_data.led_state_data.push_back(led_state);
		}
        strcpy(mcu_info_data.sn_num, mcu_dev_data[i].sn_num);

		data.push_back(mcu_info_data);
	}
	
	return 0;
}

int store_mcu_info(char *ip, monitor_data_t *data, int status)
{
    int ret = 0;
	size_t mcu_num = get_mcu_device_size();

	LockGuard lock_gurad(&mcu_info_mutex);
	for(size_t i=0; i<mcu_num; i++)
	{
		if(strcmp(mcu_dev_data[i].ip, ip)==0)
		{
			if((mcu_dev_data[i].status == HEATSDK_NET_ERROR) && (status==HEATSDK_NET_ERROR))
				break;
		
		    if(mcu_dev_data[i].status != status)
		    {
		    	DBG("mcu ware change. status=%d", status);
				mcu_dev_data[i].info_warn_chang = 1;
		    }
			if(status == HEATSDK_DEV_OK)
			{
				if((mcu_dev_data[i].data.angle_warn != data->angle_warn) 
					|| (mcu_dev_data[i].data.motion_warn != data->motion_warn) 
					|| (mcu_dev_data[i].data.heat_state != data->heat_state) 
					|| (mcu_dev_data[i].data.fan1_state != data->fan1_state) 
					|| (mcu_dev_data[i].data.fan2_state != data->fan2_state) 
					|| (mcu_dev_data[i].data.fan1_fault_state != data->fan1_fault_state) 
					|| (mcu_dev_data[i].data.fan2_fault_state != data->fan2_fault_state))
			    {
			    	DBG("mcu ware change. angle_warn=%d,motion_warn=%d,heat_state=%d,"
						"fan1_state=%d, fan2_state=%d, "
						"fan1_fault_state=%d, fan2_fault_state=%d",
						data->angle_warn, data->motion_warn, data->heat_state, data->fan1_state, 
						data->fan2_state, data->fan1_fault_state, data->fan2_fault_state);
					mcu_dev_data[i].info_warn_chang = 1;
			    }
				mcu_dev_data[i].status = status;
				mcu_dev_data[i].data.roll = data->roll;
				mcu_dev_data[i].data.pitch = data->pitch;
				mcu_dev_data[i].data.yaw = data->yaw;
				mcu_dev_data[i].data.temperature = data->temperature;
				mcu_dev_data[i].data.humidity = data->humidity;
				mcu_dev_data[i].data.voltage = data->voltage;
				mcu_dev_data[i].data.angle_warn = data->angle_warn;
				mcu_dev_data[i].data.motion_warn = data->motion_warn;
				mcu_dev_data[i].data.gyrochip_inited = data->gyrochip_inited;
				mcu_dev_data[i].data.gryo_inited = data->gryo_inited;
				mcu_dev_data[i].data.heat_state = data->heat_state;
				mcu_dev_data[i].data.fan1_state = data->fan1_state;
				mcu_dev_data[i].data.fan2_state = data->fan2_state;
				mcu_dev_data[i].data.led_state = 0;
				mcu_dev_data[i].data.led_online_state = 0;
				mcu_dev_data[i].data.led_lightness = 0;
				mcu_dev_data[i].data.data_timestamp = data->data_timestamp;
				mcu_dev_data[i].data.version = data->version;
				
				mcu_dev_data[i].data.fan1_fault_state = data->fan1_fault_state;
				mcu_dev_data[i].data.fan2_fault_state = data->fan2_fault_state;
				mcu_dev_data[i].data.angle_threshold_valueX = data->angle_threshold_valueX;
				mcu_dev_data[i].data.angle_threshold_valueY = data->angle_threshold_valueY;
				mcu_dev_data[i].data.angle_threshold_valueZ = data->angle_threshold_valueZ;
				mcu_dev_data[i].data.angle_threshold_value_quake = data->angle_threshold_value_quake;
				
				mcu_dev_data[i].data.led_data.led_online_state = data->led_data.led_online_state;
				mcu_dev_data[i].data.led_data.led_type = data->led_data.led_type;
				mcu_dev_data[i].data.led_data.photosensitive_on_value = data->led_data.photosensitive_on_value;
				mcu_dev_data[i].data.led_data.photosensitive_off_value = data->led_data.photosensitive_off_value;

				for(size_t led_i=0; led_i<data->led_data.led_state_data.size(); led_i++)
				{
					size_t led_j;
					for(led_j=0; led_j<mcu_dev_data[i].data.led_data.led_state_data.size(); led_j++)
					{
						if(data->led_data.led_state_data[led_i].led_index == 
							mcu_dev_data[i].data.led_data.led_state_data[led_j].led_index)
						{
							mcu_dev_data[i].data.led_data.led_state_data[led_j].led_lightness = data->led_data.led_state_data[led_i].led_lightness;
							mcu_dev_data[i].data.led_data.led_state_data[led_j].led_state = data->led_data.led_state_data[led_i].led_state;
							break;
						}
					}
					if(led_j == mcu_dev_data[i].data.led_data.led_state_data.size())
					{
						led_state_t led_state;
						led_state.led_index = data->led_data.led_state_data[led_i].led_index;
						led_state.led_lightness = data->led_data.led_state_data[led_i].led_lightness;
						led_state.led_state = data->led_data.led_state_data[led_i].led_state;
						mcu_dev_data[i].data.led_data.led_state_data.push_back(led_state);
					}
				}
				mcu_dev_data[i].data.fan1_fault_state = data->fan1_fault_state;
				mcu_dev_data[i].data.fan2_fault_state = data->fan2_fault_state;
			}
			else if(status == HEATSDK_NET_ERROR)
			{
				mcu_dev_data[i].status = status;
				mcu_dev_data[i].data.roll = 0;
				mcu_dev_data[i].data.pitch = 0;
				mcu_dev_data[i].data.yaw = 0;
				mcu_dev_data[i].data.temperature = 0;
				mcu_dev_data[i].data.humidity = 0;
				mcu_dev_data[i].data.voltage = 0;
				mcu_dev_data[i].data.angle_warn = 0;
				mcu_dev_data[i].data.motion_warn = 0;
				mcu_dev_data[i].data.gyrochip_inited = 0;
				mcu_dev_data[i].data.gryo_inited = 0;
				mcu_dev_data[i].data.heat_state = 0;
				mcu_dev_data[i].data.fan1_state = 0;
				mcu_dev_data[i].data.fan2_state = 0;
				mcu_dev_data[i].data.led_state = 0;
				mcu_dev_data[i].data.led_online_state = 0;
				mcu_dev_data[i].data.led_lightness = 0;
				mcu_dev_data[i].data.data_timestamp = 0;
				mcu_dev_data[i].data.version = 0;

				mcu_dev_data[i].data.fan1_fault_state = 0;
				mcu_dev_data[i].data.fan2_fault_state = 0;
				mcu_dev_data[i].data.angle_threshold_valueX = 0;
				mcu_dev_data[i].data.angle_threshold_valueY = 0;
				mcu_dev_data[i].data.angle_threshold_valueZ = 0;
				mcu_dev_data[i].data.angle_threshold_value_quake = 0;

				mcu_dev_data[i].data.led_data.led_online_state = 0;
				mcu_dev_data[i].data.led_data.led_type = 0;
				mcu_dev_data[i].data.led_data.photosensitive_on_value = 0;
				mcu_dev_data[i].data.led_data.photosensitive_off_value = 0;
				mcu_dev_data[i].data.led_data.led_state_data.clear();
			}
			
			break;
		}
	}
	
	return 0;
}

int store_led_val(char *ip, monitor_data_t *data)
{
    int ret = 0;
	size_t mcu_num = get_mcu_device_size();

	LockGuard lock_gurad(&mcu_info_mutex);
	for(size_t i=0; i<mcu_num; i++)
	{
		if(strcmp(mcu_dev_data[i].ip, ip)==0)
		{
			mcu_dev_data[i].data.led_data.led_type = data->led_data.led_type;
			mcu_dev_data[i].data.led_data.photosensitive_on_value = data->led_data.photosensitive_on_value;
			mcu_dev_data[i].data.led_data.photosensitive_off_value = data->led_data.photosensitive_off_value;

			for(size_t led_i=0; led_i<data->led_data.led_state_data.size(); led_i++)
			{
				size_t led_j;
				for(led_j=0; led_j<mcu_dev_data[i].data.led_data.led_state_data.size(); led_j++)
				{
					if(data->led_data.led_state_data[led_i].led_index == 
						mcu_dev_data[i].data.led_data.led_state_data[led_j].led_index)
					{
						mcu_dev_data[i].data.led_data.led_state_data[led_j].led_lightness = data->led_data.led_state_data[led_i].led_lightness;
						mcu_dev_data[i].data.led_data.led_state_data[led_j].led_state = data->led_data.led_state_data[led_i].led_state;
						break;
					}
				}
				if(led_j == mcu_dev_data[i].data.led_data.led_state_data.size())
				{
					led_state_t led_state;
					led_state.led_index = data->led_data.led_state_data[led_i].led_index;
					led_state.led_lightness = data->led_data.led_state_data[led_i].led_lightness;
					led_state.led_state = data->led_data.led_state_data[led_i].led_state;
					mcu_dev_data[i].data.led_data.led_state_data.push_back(led_state);
				}
			}
			break;
		}
	}
	
	return 0;
}

int get_store_sn_num(char *ip, char *sn_num)
{
    int ret = 0;
    size_t mcu_num = get_mcu_device_size();

    LockGuard lock_gurad(&mcu_info_mutex);
    for(size_t i=0; i<mcu_num; i++)
    {
        if(strcmp(mcu_dev_data[i].ip, ip)==0)
        {
            if(strlen(mcu_dev_data[i].sn_num)>0)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
    
    return 2;
}
int store_mcu_sn_num(char *ip, char *sn_num)
{
    size_t mcu_num = get_mcu_device_size();
    LockGuard lock_gurad(&mcu_info_mutex);
    for(size_t i=0; i<mcu_num; i++)
    {
        if(strcmp(mcu_dev_data[i].ip, ip)==0)
        {
            sprintf(mcu_dev_data[i].sn_num, "%s", sn_num);
            break;
        }
    }

    return 0;
}


/*设置g_c500_control_command控制命令*/
int set_controller_command(C500_CONTROL_STRU *control_command)
{
	size_t mcu_dev_i = 0;
	size_t mcu_num = get_mcu_device_size();
	
	for(mcu_dev_i=0; mcu_dev_i<mcu_num; mcu_dev_i++)
	{
		if(strcmp(mcu_dev_data[mcu_dev_i].camera_controller_guid, control_command->camera_controller_guid)==0)
		{
			break;
		}
	}
	
	LockGuard lock_gurad(&c500_control_mutex);
	if(mcu_dev_i<mcu_num)
	{
		LockGuard lock_gurad(&mcu_info_mutex);
		if(mcu_dev_data[mcu_dev_i].status == 1)
		{
			sprintf(g_c500_control_command.control_code, "%s", control_command->control_code);
			sprintf(g_c500_control_command.ip, "%s", control_command->ip);
			g_c500_control_command.comm_type = control_command->comm_type;
			g_c500_control_command.comm_value[0] = control_command->comm_value[0];
			g_c500_control_command.comm_value[1] = control_command->comm_value[1];
			g_c500_control_command.comm_value[2] = control_command->comm_value[2];
			g_c500_control_command.comm_value[3] = control_command->comm_value[3];
			g_c500_control_command.comm_value[4] = control_command->comm_value[4];
			g_c500_control_command.comm_value[5] = control_command->comm_value[5];
			g_c500_control_command.comm_value[6] = control_command->comm_value[6];
			g_c500_control_command.comm_value[7] = control_command->comm_value[7];
			g_c500_control_command.comm_result = -1;
		}
		else
		{
			g_c500_control_command.comm_result = 0;
		}
	}
	else
	{
		g_c500_control_command.comm_result = 0;
	}

	return 0;
}
/*取g_c500_control_command控制命令*/
int get_controller_command(C500_CONTROL_STRU *control_command)
{
	LockGuard lock_gurad(&c500_control_mutex);
	sprintf(control_command->control_code, "%s", g_c500_control_command.control_code);
	sprintf(control_command->ip, "%s", g_c500_control_command.ip);
	control_command->comm_type = g_c500_control_command.comm_type;
	control_command->comm_value[0] = g_c500_control_command.comm_value[0];
	control_command->comm_value[1] = g_c500_control_command.comm_value[1];
	control_command->comm_value[2] = g_c500_control_command.comm_value[2];
	control_command->comm_value[3] = g_c500_control_command.comm_value[3];
	control_command->comm_value[4] = g_c500_control_command.comm_value[4];
	control_command->comm_value[5] = g_c500_control_command.comm_value[5];
	control_command->comm_value[6] = g_c500_control_command.comm_value[6];
	control_command->comm_value[7] = g_c500_control_command.comm_value[7];
	control_command->comm_result = g_c500_control_command.comm_result;
	
	return 0;
}

/*取c500控制命令处理结果*/
int get_controller_process_result(void)
{
	LockGuard lock_gurad(&c500_control_mutex);
	int ret = g_c500_control_command.comm_result;

	return ret;
}
int get_controller_process_result_msg(char *msg)
{
	LockGuard lock_gurad(&c500_control_mutex);
	int ret = g_c500_control_command.comm_result;
	sprintf(msg, "%s", g_c500_control_command.comm_msg);
	return ret;
}

//添加c500升级任务，返回值：1此任务已存在，0添加成功
int add_c500_upgrade_task(const char *ip, const char *bin_path, 
		const char* current_software_version, const char* target_software_version)
{
	int ret = 0;
	LockGuard lock_gurad(&c500_upgrade_mutex);
	size_t tmp_i = 0;
	for(tmp_i=0; tmp_i<g_c500_upgrade_task.size(); tmp_i++)
	{
		if(strcmp(ip, g_c500_upgrade_task[tmp_i].ip) == 0)
		{
			DBG("矩阵控制器%s正在进行升级任务", ip);
			return 1;
		}
	}

	if(tmp_i >= g_c500_upgrade_task.size())
	{
		DBG("添加矩阵控制器%s升级任务", ip);
		C500_UPGRADE_STRU upgrade_data;
		snprintf(upgrade_data.ip, 32, "%s", ip);
		snprintf(upgrade_data.file_path, 128, "%s", bin_path);
		snprintf(upgrade_data.current_software_version, 16, "%s", current_software_version);
		snprintf(upgrade_data.target_software_version, 16, "%s", target_software_version);
		upgrade_data.state = C500_UPGRADE_WAIT;
		upgrade_data.step_total = 6;
		upgrade_data.current_step = 1;
		upgrade_data.progress = 10;
		snprintf(upgrade_data.message, 64, "添加矩阵控制器");
		g_c500_upgrade_task.push_back(upgrade_data);
	}

	return 0;
}

//删除c500升级任务，返回值：1删除成功
int del_c500_upgrade_task(const char *ip)
{
	int ret = 0;
	LockGuard lock_gurad(&c500_upgrade_mutex);
	size_t tmp_i = 0;
	for(auto it = g_c500_upgrade_task.begin(); it != g_c500_upgrade_task.end();)
	{
		if(strcmp(ip, it->ip) == 0)
		{
			it = g_c500_upgrade_task.erase(it);
			DBG("矩阵控制器%s删除升级任务", ip);
			return 1;
		}
		++it; //指向下一个位置
	}

	return 0;
}

int update_c500_upgrade_state(const char *ip, C500_UPGRADE_STATE state, int step, 
					int progress, const char *msg)
{
	int ret = 0;
	LockGuard lock_gurad(&c500_upgrade_mutex);
	size_t tmp_i = 0;
	for(tmp_i=0; tmp_i<g_c500_upgrade_task.size(); tmp_i++)
	{
		if(strcmp(ip, g_c500_upgrade_task[tmp_i].ip) == 0)
		{
			g_c500_upgrade_task[tmp_i].state = state;
			g_c500_upgrade_task[tmp_i].current_step = step;
			g_c500_upgrade_task[tmp_i].progress = progress;
			snprintf(g_c500_upgrade_task[tmp_i].message, 64, "%s", msg);
			DBG("update_c500_upgrade_state msg:%s", msg);
			break;
		}
	}

	if(tmp_i >= g_c500_upgrade_task.size())
	{
		ERR("没找到矩阵控制器%s升级任务", ip);
	}

	return 0;
}
//获取c500升级状态  。返回值：0正常放回，-1没找到此升级任务
int get_c500_upgrade_state(const char* ip, C500_UPGRADE_STATE* state,
		int* step_total, int* current_step, int* progress, char* message)
{
	int ret = 0;
	LockGuard lock_gurad(&c500_upgrade_mutex);
	size_t tmp_i = 0;
	for(tmp_i=0; tmp_i<g_c500_upgrade_task.size(); tmp_i++)
	{
		if(strcmp(ip, g_c500_upgrade_task[tmp_i].ip) == 0)
		{
			*state = g_c500_upgrade_task[tmp_i].state;
			*step_total = g_c500_upgrade_task[tmp_i].step_total;
			*current_step = g_c500_upgrade_task[tmp_i].current_step;
			*progress = g_c500_upgrade_task[tmp_i].progress;
			snprintf(message, 64, "%s", g_c500_upgrade_task[tmp_i].message);
			break;
		}
	}

	if(tmp_i >= g_c500_upgrade_task.size())
	{
		DBG("没找到矩阵控制器%s升级任务", ip);
		return -1;
	}

	return 0;
}

//取升级任务 
C500_UPGRADE_STATE find_c500_upgrade_task(char *ip, char *bin_path)
{
	LockGuard lock_gurad(&c500_upgrade_mutex);
	size_t tmp_i = 0;
	for(tmp_i=0; tmp_i<g_c500_upgrade_task.size(); tmp_i++)
	{
		if(strcmp(ip, g_c500_upgrade_task[tmp_i].ip) == 0)
		{
			snprintf(bin_path, 128, "%s", g_c500_upgrade_task[tmp_i].file_path);
			return g_c500_upgrade_task[tmp_i].state;
		}
	}

	return C500_NO_UPGRADE_TASK;
}

static int mysocketpair(int fds[2])
{
    fds[0] = -1;
    fds[1] = -1;
    int sock_type = SOCK_DGRAM;
    int fd = socket(AF_INET, sock_type, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = 0;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0) {
        close(fd);
        perror("bind");
        return -1;
    }
    if (sock_type != SOCK_DGRAM) {
        if (listen(fd, 1) < 0) {
            close(fd);
            perror("listen");
            return -1;
        }
    }
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int ret = getsockname(fd, (struct sockaddr *)&servaddr, &addrlen);
    if (ret < 0) {
        perror("getsockname");
        close(fd);
        return -1;
    }
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addrlen = sizeof(struct sockaddr_in);

    int client_fd = socket(AF_INET, sock_type, 0);
    if (connect(client_fd, (struct sockaddr*)&servaddr, addrlen) < 0) {
        close(fd);
        perror("connect");
        close(client_fd);
        return -1;
    }
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));

    fds[0] = fd;
    fds[1] = client_fd;
    return 0;
}


static int writen(int fd, const void *buf, int n)
{
    if (fd < 0 || n <= 0) {
        return -1;
    }
    char *ptr = (char *)buf;
    int nwrite = 0;
    int ret = 0;
    do {
        ret = send(fd, ptr+nwrite, n-nwrite, 0);
        if (ret > 0) {
            nwrite += ret;
            if (nwrite >= n) {
                break;
            }
        }
#ifdef __MINGW32__
    } while (ret > 0 || (ret < 0 && WSAGetLastError() == WSAEINTR));
#else
    } while (ret > 0 || (ret < 0 && errno == EINTR));
#endif
    return nwrite;
}


static int readn(int fd, void *buf, int n)
{
    if (fd < 0 || n <= 0) {
        return -1;
    }
    int nread = 0;
    int ret = 0;
    char *ptr = (char *)buf;
    int err_cnt = 0;
    do {
        ret = recv(fd, ptr+nread, n-nread, 0);
        if (ret > 0) {
            nread += ret;
            if (nread >= n) {
                break;
            }
        } else {
            err_cnt++;
        }
#ifdef __MINGW32__
    } while (ret > 0 || (ret < 0 && WSAGetLastError() == WSAEINTR));
#else
    } while (ret > 0 || (ret < 0 && errno == EINTR));
#endif
    return nread;
}

static unsigned char xCal_crc(unsigned char *ptr,unsigned int len)
{
    unsigned char crc = 0;
    unsigned char i;
    crc = 0;
    while(len--)
    {
       crc ^= *ptr++;
       for(i = 0;i < 8;i++)
       {
           if(crc & 0x01)
           {
               crc = (crc >> 1) ^ 0x8C;
           }else crc >>= 1;
       }
    }
    return crc;
}

/*取一组正确数据
*	返回值：0 没接收到正确数据，1 接收到1组正确数据，
*			-1 出现错误
*/
static int read_check(int fd, unsigned char *buf, int *data_l)
{
	int data_len = 0;
	int ret = 0;
	unsigned char receive_buf[64] = {0};

	if(buf == NULL)
	{
		ERR("buf null");
		return -1;
	}

	while(receive_buf[0] != 0x88)
	{
		ret =readn(fd, receive_buf, 1); 
		if (ret != 1) {
	        ERR("fd%d read error, recv len: %d", fd, ret);
	        return -1;
	    }
	}
	ret =readn(fd, receive_buf+1, 2); 
	if (ret != 2) {
        ERR("fd%d read error, recv len: %d", fd, ret);
        return -1;
    }
	data_len = receive_buf[2];
	if((data_len>0) && (data_len<60))
	{
		ret =readn(fd, receive_buf+3, data_len+1); 
		if (ret != data_len+1) {
	        ERR("fd%d read error, recv len: %d", fd, ret);
	        return -1;
	    }
		unsigned short cksum = 0;
        int temp_i;
        for (temp_i = 0; temp_i < data_len+3; temp_i++) 
		{
            cksum += receive_buf[temp_i];
        }
        if(receive_buf[data_len+3] == (cksum&0xff))
		{
		//	DBG("fd%d 接收数据校验完成", fd);
			for(temp_i=0; temp_i<data_len+4; temp_i++)
			{
			//	DBG("fd%d receive_buf[%d] = %02x", fd, temp_i, receive_buf[temp_i]);
				buf[temp_i] = receive_buf[temp_i];
			}
			*data_l = data_len;
		}
		else
		{
			DBG("fd%d c500接收数据校验错误,receive=%02x,cksum=%02x", fd, receive_buf[data_len+3], cksum);
			return 0;
		}
	}
	else
	{
		ERR("fd%d read err, recv: %02x %02x %02x", fd, receive_buf[0], receive_buf[1], receive_buf[2]);
		return 0;
	}


	return 1;
}

int creat_tcp_sock(const char *ip, int port)
{
    if (ip) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return -1;
        }
        struct sockaddr_in server_addr;
        socklen_t server_addr_len = sizeof(struct sockaddr_in);
        memset(&server_addr, 0, sizeof(struct sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);


#ifdef SO_NOSIGPIPE
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
#endif

        if (connect(sock, (struct sockaddr*)&server_addr, server_addr_len) < 0) {
            close(sock);
            return -4;
        }

        return sock;
    }
    return -5;
}

static long long getTimeNow()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long now = tv.tv_sec*1000LL + tv.tv_usec/1000LL;
    return now;
}

static void monitor_read_data(monitor_client_t *client, unsigned char *buf)
{
    client->data.version = ((buf[17]<<8)|(buf[18]&0xff));
    unsigned char flags = buf[16];
    pthread_mutex_lock(&client->data.data_mutex);
    client->data.roll = ((short)((buf[3]<<8)|buf[4]))*1.0f/10.0f;
    client->data.pitch = ((short)((buf[5]<<8))|buf[6])*1.0f/10.0f;
    client->data.yaw = ((short)((buf[7]<<8)|buf[8]))*1.0f/10.0f;
    client->data.temperature = ((short)((buf[9]<<8)|buf[10]))*1.0f/10.0f;
    client->data.humidity = ((short)((buf[11]<<8)|buf[12]))*1.0f/10.0f;
    client->data.voltage = ((short)((buf[13]<<8)|buf[14]))*1.0f/100.0f;

    char pre_angle_warn = client->data.angle_warn;
    client->data.angle_warn = (flags&1);
    char pre_motion_warn = client->data.motion_warn;
    client->data.motion_warn = ((flags&2)>>1);
    client->data.gyrochip_inited = ((flags&4)>>2);
    client->data.gryo_inited = ((flags&8)>>3);
    char pre_heat_state = client->data.heat_state;
    client->data.heat_state = ((flags&0x10)>>4);

	char pre_fan1_state = client->data.fan1_state;
	client->data.fan1_state = ((flags&0x20)>>5);
	char pre_fan2_state = client->data.fan2_state;
	client->data.fan2_state = ((flags&0x40)>>6);
//	char pre_led_state = client->data.led_state;
//	client->data.led_state = ((flags&0x80)>>7);
//	char pre_led_online_state = client->data.led_online_state;
//	client->data.led_online_state = ((flags&0x0100)>>8);
	client->data.led_state = 0;
	client->data.led_online_state = 0;
	flags = buf[15];
	char pre_led_online_state = client->data.led_data.led_online_state;
	client->data.led_data.led_online_state = (flags&0x01);
	char pre_fan1_fault_state = client->data.fan1_fault_state;
	client->data.fan1_fault_state = ((flags&2)>>1);
	char pre_fan2_fault_state = client->data.fan2_fault_state;
	client->data.fan2_fault_state = ((flags&4)>>1);
	
    client->data.data_timestamp = getTimeNow();
    pthread_mutex_unlock(&client->data.data_mutex);

    int status_change = 0;
    if (pre_angle_warn != client->data.angle_warn) {
        status_change = 1;
    }

    if (pre_motion_warn != client->data.motion_warn) {
        status_change = 1;
    }

    if (client->data.gryo_inited && client->data.gyrochip_inited) {
    } else {
    }

    if (pre_heat_state != client->data.heat_state) {
        status_change = 1;
    }

    if (status_change) {
//        if (glob_cb) {
//            glob_cb(1, client->ip, &client->data);
//        }
    }
}

static int process_monitor_data_client(int fd, monitor_client_t *client)
{
    unsigned char buf[128] = {0};
    int n = readn(fd, buf, 2);
    monitor_data_t empty_data;
    memset(&empty_data, 0, sizeof(monitor_data_t));
    if (n != 2) {
        DBG("recv client data failed return: %d", n);
        return -1;
    }

    unsigned char tag = buf[0];
    unsigned char type = buf[1];
    if ((buf[0] == '#' && buf[1] == '#')) { // upgrading
        return 2;
    } else if ((buf[0] == '*' && buf[1] == '*')) { // booting
        client->status = 0;
        return 0;
    }
    if (tag != 0x88) { // tag error
        return -1;
    }
    if (type == 0xa4 || type == 0xa5 || type == 0xa2) { // upgrading
        return 2;
    }

    switch (type) {
    case 0xa0:
    {
        int ret = readn(fd, buf+n, 1);
        if (ret != 1) { // read len error
            return -2;
        }
        n += 1;
        int data_len = buf[2];
        if (data_len != 16) {
            return -2;
        }
        ret = readn(fd, buf+n, data_len+1);
        n += ret;
        if (ret == data_len+1) {
            unsigned short cksum = 0;
            int i;
            for (i = 0; i < n; i++) {
                if (i != n-1) {
                    cksum += buf[i];
                }
            }
            cksum = cksum&0xff;
            if (cksum != buf[n-1]) {
                DBG("warning, cksum error!!!");
            }
		//	for(int temp_i=0; temp_i<n; temp_i++)
		//	{
		//		DBG("fd%d buf[%d] = %02x", fd, temp_i, buf[temp_i]);
		//	}
            monitor_read_data(client, buf);
            return 1;
        } else {
            return -2;
        }
        break;
    }
    default:
        return -3;
        break;
    }
    return 0;
}

// 开始加热
static int sendHeatCmd(int fd, int heat)
{
    int n = 8;
    int data_len = 5;
    char buf[128] = {};
    buf[0] = 0x88;
    buf[1] = 0xb2;
    buf[2] = data_len;
    buf[3] = heat;
    buf[4] = heat;
    buf[5] = 0xFF;
    buf[6] = 0xFF;
	buf[7] = 0xFF;

    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -2; // send upgrade cmd failed;
    }
    return 0;
}
static int sendHeatCmd1(int fd, int heat)
{
    int n = 8;
    int data_len = 5;
    char buf[128] = {};
    buf[0] = 0x88;
    buf[1] = 0xb2;
    buf[2] = data_len;
    buf[3] = heat;
    buf[4] = heat;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -2; // send upgrade cmd failed;
    }
    return 0;
}
//继电器重启 //交换机重启
static int sendRebootCmd(int fd)
{
    int n = 8;
    int data_len = 5;
    char buf[128] = {};
    buf[0] = 0x88;
    buf[1] = 0xb2;
    buf[2] = data_len;
    buf[3] = 0xFF;
    buf[4] = 0xFF;
    buf[5] = 1;
    buf[6] = 0xFF;
    buf[7] = 0xFF;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -2; // send upgrade cmd failed;
    }
    return 0;
}

// 角度重置
static int sendResetCmd(int fd)
{
    int n = 8;
    int data_len = 5;
    char buf[128] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb2;
    buf[2] = data_len;
	buf[3] = 0xFF;
    buf[4] = 0xFF;
    buf[5] = 0xFF;
    buf[6] = 0xFF;
    buf[7] = 1;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -2; // send upgrade cmd failed;
    }
    return 0;
}
//角度阈值设定
static int sendAngleThresholdSet(int fd, double *setValue)
{
    int n = 11;
    int data_len = 8;
    unsigned char buf[128] = {0};
	unsigned char recvbuf[64] = {0};
	
    buf[0] = 0x88;
    buf[1] = 0xb3;
    buf[2] = data_len;
	buf[3] = (int)(setValue[0]*10) & 0xFF00;
    buf[4] = (int)(setValue[0]*10) & 0x00FF;
    buf[5] = (int)(setValue[1]*10) & 0xFF00;
    buf[6] = (int)(setValue[1]*10) & 0x00FF;
    buf[7] = (int)(setValue[2]*10) & 0xFF00;
    buf[8] = (int)(setValue[2]*10) & 0x00FF;
    buf[9] = (int)(setValue[3]*10) & 0xFF00;
    buf[10] = (int)(setValue[3]*10) & 0x00FF;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }

	for(int tmp_i=0; tmp_i<n; tmp_i++)
	{
		DBG("fd%d sendbuf[%d] = %02x", fd, tmp_i, buf[tmp_i]);
	}

	ret = read_check(fd, recvbuf, &data_len);
	if(ret != 1)
	{
		return -1;
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
	{
		DBG("fd%d 接收到心跳数据，重新接收", fd);
		ret = read_check(fd, recvbuf, &data_len);
		if(ret != 1)
		{
			return -1;
		}
	}

    if (recvbuf[3] == buf[3] && recvbuf[0] == 0x88 && recvbuf[1] == 0xa3) {
        return 0;
    }
    ERR("AngleThresholdSet error, recv data: %02x, %02x, %02x, %02x", buf[0], buf[1],buf[2], buf[3]);
    return -1;
}
//角度阈值获取
static int angleThresholdGet(int fd, monitor_data_t &data)
{
    int data_len = 1;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xc1;
    buf[2] = data_len;
    buf[3] = 1;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }

	ret = read_check(fd, recvbuf, &data_len);
	if(ret != 1)
	{
		return -1;
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
	{
		DBG("fd%d 接收到心跳数据，重新接收", fd);
		ret = read_check(fd, recvbuf, &data_len);
		if(ret != 1)
		{
			return -1;
		}
	}

    if (recvbuf[0] == 0x88 && recvbuf[1] == 0xd1) {
		data.angle_threshold_valueX = ((recvbuf[9]*0x100)+recvbuf[10])*1.0/10.0;
		data.angle_threshold_valueY = ((recvbuf[7]*0x100)+recvbuf[8])*1.0/10.0;
		data.angle_threshold_valueZ = ((recvbuf[5]*0x100)+recvbuf[6])*1.0/10.0;
		data.angle_threshold_value_quake = ((recvbuf[3]*0x100)+recvbuf[4])*1.0/10.0;
		
		DBG("angleThresholdGet receive data: x %4.1f, y %4.1f, z %4.1f, quake %4.1f",  
				data.angle_threshold_valueX, data.angle_threshold_valueY, 
				data.angle_threshold_valueZ, data.angle_threshold_value_quake);
		return 0;
    }
    ERR("angleThresholdGet error, recv data: %02x, %02x, %02x, %02x", 
			recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -2;
}

static int sendLedOnOffCmd(int fd, int onoff)
{
    int data_len = 1;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb8;
    buf[2] = data_len;
    buf[3] = onoff;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
    ret = readn(fd, recvbuf, n);
    if (ret != n) {
        ERR("ledonoff error, recv len: %d", ret);
        return -1;
    }
    if (recvbuf[0] == 0x88 && recvbuf[1] == 0xa0) {
        int remain = 16 - data_len;
        readn(fd, recvbuf, remain);
        ret = readn(fd, recvbuf, n);
        if (ret != n) {
            ERR("ledonoff error2, recv len: %d", ret);
            return -1;
        }
    }
    if (recvbuf[3] == onoff && recvbuf[0] == 0x88 && recvbuf[1] == 0xa8) {
        return 0;
    }
    ERR("ledonoff error, recv data: %02x, %02x, %02x, %02x", recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -1;
}

static int sendFanOnOffCmd(int fd, int onoff)
{
    int data_len = 1;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xba;
    buf[2] = data_len;
    buf[3] = onoff;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
    ret = readn(fd, recvbuf, n);
    if (ret != n) {
        ERR("fanonoff error, recv len: %d", ret);
        return -1;
    }
    if (recvbuf[0] == 0x88 && recvbuf[1] == 0xa0) {
        int remain = 16 - data_len;
        readn(fd, recvbuf, remain);
        ret = readn(fd, recvbuf, n);
        if (ret != n) {
            ERR("fanonoff error2, recv len: %d", ret);
            return -1;
        }
    }
    if (recvbuf[3] == onoff && recvbuf[0] == 0x88 && recvbuf[1] == 0xaa) {
        return 0;
    }
    ERR("fanonff error, recv data: %02x, %02x, %02x, %02x", recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -1;
}

static int SNNumGet(int fd, char *sn_num)
{
    int data_len = 1;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xbc;
    buf[2] = data_len;
    buf[3] = 1;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
    memset(recvbuf, 0, 64);
    ret = read_check(fd, recvbuf, &data_len);
    if(ret != 1)
    {
        return -1;
    }
    if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
    {
        DBG("fd%d 接收到心跳数据，重新接收", fd);
        ret = read_check(fd, recvbuf, &data_len);
        if(ret != 1)
        {
            return -1;
        }
    }
//    DBG("data_len=%d", data_len);
//    for(int i=0; i<61; i++)
//    {
//        DBG("receive[%d] : %02x", i, recvbuf[i]);
//    }
//    if(recvbuf[0] == 0x88 && recvbuf[1] == 0xac && data_len==44)
    if(recvbuf[0] == 0x88 && recvbuf[1] == 0xac)
    {
        char tmp_num[64] = {0};
        int tmp_j = 0;
        for(tmp_j=0; tmp_j<32; tmp_j++)
        {
            tmp_num[tmp_j] = recvbuf[15 + tmp_j];
        //    DBG("sn:%02x", tmp_num[tmp_j]);
        }

        sprintf(sn_num, "%s", tmp_num);
        DBG("sn_num:%s", sn_num);
        
        return 1;
    }

    ERR("recv data: %02x, %02x, %02x, %02x", recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -2;

}

static int ledGetVal(int fd, monitor_data_t &data)
{
    int data_len = 1;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb9;
    buf[2] = data_len;
    buf[3] = 1;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }

	ret = read_check(fd, recvbuf, &data_len);
	if(ret != 1)
	{
		return -1;
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
	{
		DBG("fd%d 接收到心跳数据，重新接收", fd);
		ret = read_check(fd, recvbuf, &data_len);
		if(ret != 1)
		{
			return -1;
		}
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa9)
	{
		if(recvbuf[2] == 1)
		{//单补光灯版本通信20200706
			data.led_data.led_type = 55;
			data.led_data.photosensitive_on_value = 0;
			data.led_data.photosensitive_off_value = 0;
			led_state_t led_state;
			led_state.led_index = 1;
			led_state.led_lightness = recvbuf[3];
			led_state.led_state = data.led_state;
			data.led_data.led_state_data.push_back(led_state);
			DBG("fd%d 单组补光灯版本协议ledget receive data %d: %d, %d", fd, 
					led_state.led_index, led_state.led_lightness, led_state.led_state);
			DBG("fd%d ledget receive data: type %d, on %d, off %d", fd, data.led_data.led_type, 
					data.led_data.photosensitive_on_value, data.led_data.photosensitive_off_value);
			return 1;
		}
		else if(recvbuf[2] == 11)
		{//多组补光灯20210305版协议
			data.led_data.led_type = recvbuf[9];
			data.led_data.photosensitive_on_value = recvbuf[10]*0x100 + recvbuf[11];
			data.led_data.photosensitive_off_value = recvbuf[12]*0x100 + recvbuf[13];
			led_state_t led_state;
			int led_num = 3;
//			if(data.led_data.led_type == 55)
//				led_num = 1;
			for(int led_i=0; led_i<led_num; led_i++)
			{
				led_state.led_index = led_i + 1;
				led_state.led_lightness = recvbuf[3+led_i*2];
				led_state.led_state = recvbuf[3+1+led_i*2];
				data.led_data.led_state_data.push_back(led_state);
				DBG("fd%d ledget receive data %d: %d, %d", fd, 
						led_state.led_index, led_state.led_lightness, led_state.led_state);
			}
			DBG("fd%d ledget receive data: type %d, on %d, off %d", fd, data.led_data.led_type, 
					data.led_data.photosensitive_on_value, data.led_data.photosensitive_off_value);
			return 0;
		}
	}

    ERR("ledgt error, recv data: %02x, %02x, %02x, %02x", recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -2;
}

static int ledSetVal(int fd, int val1, int val2, int val3)
{
    int data_len = 3;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb7;
    buf[2] = data_len;
    buf[3] = val1;
	buf[4] = val2;
	buf[5] = val3;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
	for(int temp_i=0; temp_i<n; temp_i++)
	{
		DBG("ledSetVal sendbuf[%d] = %02x", temp_i, buf[temp_i]);
	}

	ret = read_check(fd, recvbuf, &data_len);
	if(ret != 1)
	{
		return -1;
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
	{
		DBG("fd%d 接收到心跳数据，重新接收", fd);
		ret = read_check(fd, recvbuf, &data_len);
		if(ret != 1)
		{
			return -1;
		}
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa7)
	{
		if(recvbuf[2] == 1)
		{//单补光灯版本通信
			if((buf[3]==0xff) || (buf[3]==recvbuf[3]))
				return 0;
		}
		else if(recvbuf[2] == 9)
		{//多组补光灯20210305版协议
			if(((buf[3]==0xff) || (buf[3]==recvbuf[3]))
				&& ((buf[4]==0xff) || (buf[4]==recvbuf[4]))
				&& ((buf[5]==0xff) || (buf[5]==recvbuf[5])))
	        	return 0;
		}
	}

    ERR("ledst error, recv data: %02x, %02x, %02x, %02x, %02x, %02x", 
    		recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3], recvbuf[4], recvbuf[5]);
    return -1;
}

static int ledThresholdSet(int fd, int on_val, int off_val)
{
    int data_len = 4;
    int n = data_len + 3;
    unsigned char buf[64] = {0};
    unsigned char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xbf;
    buf[2] = data_len;
    buf[3] = on_val/0x100;
	buf[4] = on_val%0x100;
	buf[5] = off_val/0x100;
	buf[6] = off_val%0x100;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
	for(int temp_i=0; temp_i<n; temp_i++)
	{
		DBG("fd%d ledThresholdSet sendbuf[%d] = %02x", fd, temp_i, buf[temp_i]);
	}

	ret = read_check(fd, recvbuf, &data_len);
	if(ret != 1)
	{
		return -1;
	}
	if(recvbuf[0] == 0x88 && recvbuf[1] == 0xa0)
	{
		DBG("fd%d 接收到心跳数据，重新接收", fd);
		ret = read_check(fd, recvbuf, &data_len);
		if(ret != 1)
		{
			return -1;
		}
	}

    if (recvbuf[0] == 0x88 && recvbuf[1] == 0xaf) {
		if((buf[3]==recvbuf[6]) && (buf[4]==recvbuf[7]) 
			&& (buf[5]==recvbuf[8]) && (buf[6]==recvbuf[9]))
        	return 0;
    }
    ERR("ledthresholdset error, recv data: %02x, %02x, %02x, %02x", 
					recvbuf[0], recvbuf[1],recvbuf[2], recvbuf[3]);
    return -1;
}

static int sendRecvChangeIp(int fd, const char * ip)
{
    int n = 7;
    int data_len = 4;
    char buf[64] = {0};
    char recvbuf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb6;
    buf[2] = data_len;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d);

    buf[3] = a;
    buf[4] = b;
    buf[5] = c;
    buf[6] = d;

    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }
    ret = readn(fd, recvbuf, n);
    if (ret != n) {
        return -1;
    }
    if (buf[3] == recvbuf[3] && buf[4] == recvbuf[4] && buf[5] == recvbuf[5] && buf[6] == recvbuf[6]) {
        return 0;
    }
    return -1;
}


static int md5_check(const char *path)
{
    unsigned char digest[16] = {0};
    unsigned char buf[1024] = {0};
    if (path == NULL) {
        return 0;
    }
    MD5_CTX ctx;
    MD5Init(&ctx);
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 0;
    }
    int end_flag = 0;
    while (1) {
        int n = fread(buf, 1, sizeof(buf), f);
        if (feof(f) || n != sizeof(buf)) {
            end_flag = 1;
        }
        if (n > 0) {
            MD5Update(&ctx, buf, (unsigned int)n);
        }
        if (end_flag != 0) {
            break;
        }
    }
    fclose(f);
    MD5Final(&ctx, digest);

    int i;
    char file_md5[128] = {0};
    for (i = 0; i < 16; i++) {
        sprintf(file_md5+2*i, "%02X", digest[i]);
    }
    char md5_filepath[1024] = {0};
    const char *p = strrchr(path, '.');
    for (i = 0; i < (int)sizeof(md5_filepath)-1-3; i++) {
        if (path[i] == 0) {
            break;
        }
        if (p != NULL && p == &path[i]) {
            break;
        }
        md5_filepath[i] = path[i];
    }
    strcat(md5_filepath, ".txt");

    FILE *checkf = fopen(md5_filepath, "rb");
    if (checkf == NULL) {
        return 0;
    }
    while (1) {
        memset(buf, 0, sizeof(buf));
        char *p = fgets((char *)buf, sizeof(buf)-1, checkf);
        if (p == 0 || feof(checkf) || ferror(checkf)) {
            break;
        }
        char *md5_start = strstr((char*)buf, "MD5: ");
        if (md5_start) {
            char *check_md5 = md5_start+5;
            if (strncmp(check_md5, file_md5, strlen(file_md5)) == 0) {
                fclose(checkf);
                return 1;
            } else {
            }
        }
    }
    fclose(checkf);
    return 0;
}

static int monitor_send_binary(int fd, const unsigned char *data, unsigned short data_len)
{
    unsigned char *buf = (unsigned char *)malloc(data_len+64);
    buf[0] = 0x88;
    buf[1] = 0xb5;
    buf[2] = (data_len>>8)&0xff;
    buf[3] = (data_len&0xff);
    memcpy(&buf[4], data, data_len);
    int n = data_len+4;
    unsigned char crc = xCal_crc(&buf[4], n-4);
    buf[n++] = crc;
    int ret = writen(fd, buf, n);

    if (ret != n) {
        free(buf);
        DBG("send bin failed");
        return -1;
    }
    memset(buf, 0, data_len+64);
    int rsp_len = 9;
    ret = readn(fd, buf, 1);
    if (ret != 1) {
        DBG("recv bin rsp failed");
        return -1;
    }
    if (buf[0] == 0x88) {
        ret = readn(fd, buf, rsp_len-1);
        if (ret != rsp_len-1) {
            free(buf);
            DBG("recv bin rsp failed");
            return -1;
        }
    } else {
        ret = readn(fd, buf, rsp_len-3);
        if (ret != rsp_len-3) {
            free(buf);
            DBG("recv bin rsp failed");
            return -1;
        }
    }

    free(buf);
    return n;
}

static int monitor_recv_update_result(int fd)
{
    unsigned char buf[9] = {0};
    int ret = readn(fd, buf, 9);
    if (ret < 0) {
        return -1;
    }
//    0x88 0xb2
//    0x00 0x04
//    0x00 0x00 0x00 0x01
//    0xaa
    if (buf[0] == 0x88 && buf[1] == 0xa2 && buf[3] == 0x04 && buf[7] == 0x01) {
        return 0;
    } else {
        DBG("recv update result, update failed:");
        return -2;
    }
}

static int monitor_send_filesize(int fd, int filesize)
{
    DBG("send file size %d to fd: %d", filesize, fd);
    unsigned char buf[64] = {0};
    int filesize1 = htonl(filesize);
    unsigned short data_len = 4;
    buf[0] = 0x88;
    buf[1] = 0xb4;
    buf[2] = (data_len>>8)&0xff;
    buf[3] = (data_len&0xff);
    memcpy(&buf[4], &filesize1, 4);
    int n = 8;
    unsigned char crc = xCal_crc(&buf[4], n-4);
    buf[n++] = crc;

    // skip ##
    unsigned char head[2] = {0};
    {
        int ret = readn(fd, head, 2);
        if (ret != 2) {
            DBG("read head failed");
            return -1;
        }
//        // 忽略两个#
//        while (1) {
//            memset(head, 0, 2);
//            int ret = readn(fd, head, 2);
//            if (ret != 2) {
//                DBG("read head failed2");
//                return -1;
//            }
//            if (head[0] == '#' && head[1] == '#') {
//                break;
//            }
//        }
    }

    int ret = writen(fd, buf, n);
    if (ret != n) {
        return -1;
    }

    // 接收回应
    if (head[0] == '#' && head[1] == '#') {
        int ret = readn(fd, buf, n);
        if (ret == n) {
            return 0;
        }
    } else if (head[0] == 0x88 && head[1] == 0xa4) {
        int ret = readn(fd, buf, n-2);
        if (ret == (n-2)) {
            return 0;
        }
    } else if (head[0] == 0x00 && head[1] == 0x04){
        int ret = readn(fd, buf, n-4);
        if (ret == (n-4)) {
            return 0;
        }
    }
    return -1;
}

static heatsdk_client_t* heatsdk_client_search(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip)) {
            heatsdk_client_t *ret = &g_c500_list[i];
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return ret;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return NULL;
}


void heatsdk_client_close_internal(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip)) {
            close(g_c500_list[i].msg_fd);
            close(g_c500_list[i].msg_cli_fd);
            g_c500_list[i].msg_fd = -1;
            g_c500_list[i].msg_cli_fd = -1;
            g_c500_list[i].ip[0] = 0;
            g_c500_list[i].conn_ok    = -1;
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return;
}

static void sendRecvUpgrade(int* fd, const char *path, const char *ip)
{
#if 0 //eoc升级删除
    if (md5_check(path) != 1) {
        DBG("dev %s upgrade failed, file md5 check failed", ip);
        return;
    }
#endif
    /////发送升级命令/////
    int n = 8;
    int data_len = 5;
    char buf[64] = {0};
    buf[0] = 0x88;
    buf[1] = 0xb2;
    buf[2] = data_len;
	buf[3] = 0xFF;
    buf[4] = 0xFF;
    buf[5] = 0xFF;
    buf[6] = 1;
	buf[7] = 0xFF;
    if (n > 0) {
        unsigned short cksum = 0;
        int i;
        for (i = 0; i < n; i++) {
            cksum += buf[i];
        }
        buf[n] = (cksum&0xff);
        n++;
    }
    int ret = writen((*fd), buf, n);
    if (ret != n) {
        DBG("dev %s upgrade failed, send cmd failed", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 2, 25, "发送升级命令失败");
        return;
    }

	update_c500_upgrade_state(ip, C500_UPGRADE_DOING, 2, 25, "发送升级命令");

    /////等待升级模式////
    int waitcnt = 0;
    while(1) {
        char head[2] = {0};
        if ((*fd) >= 0) {
            ret = readn((*fd), head, 2);
            if (ret <= 0) {
                close((*fd)); (*fd) = -1;
            }
        }
        if ((*fd) < 0) {
            (*fd) = creat_tcp_sock(ip, 5000);
        }
        if (head[0] == '#' && head[1] == '#') {
			DBG("dev %s into bootloader mode", ip);
            break;
        }
        waitcnt++;
        sleep(1);
        if (waitcnt > 50) {
            if ((*fd) >= 0) {
                close((*fd)); (*fd) = -1;
            }
            break;
        }
    }
    if ((*fd) < 0) {
        DBG("dev %s upgrade failed, enter boot mode failed", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 3, 40, "进入boot模式失败");
        return;
    }
	update_c500_upgrade_state(ip, C500_UPGRADE_DOING, 3, 40, "进入boot模式");
    ////发送文件大小////
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        close((*fd)); (*fd) = -1;
        DBG("dev %s upgrade failed, open file failed1", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 4, 50, "读取文件失败");
        return;
    }
    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (filesize <= 0 || filesize >= 2*1024*1024) {
        fclose(f);
        close((*fd)); (*fd) = -1;
        DBG("dev %s upgrade failed, open file failed2", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 4, 50, "文件过大");
        return;
    }
    ret = monitor_send_filesize((*fd), filesize);
    if (ret < 0) {
        fclose(f);
        close((*fd)); (*fd) = -1;
        DBG("dev %s upgrade failed, send file cmd error1", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 4, 50, "发送文件大小失败");
        return;
    }

	update_c500_upgrade_state(ip, C500_UPGRADE_DOING, 4, 50, "发送文件大小");

    ///发送升级文件////
    int buf_len = 512;
    int tot_send = 0;
    unsigned char filebuf[1024] = {0};
    int end_flag = 0;
    while (1) {
        int n = fread(filebuf, 1, buf_len,f);
        if (n != buf_len) {
            if (!feof(f)) {
                end_flag = -1;
            }
        }
        tot_send += n;

        if (feof(f)) {
            end_flag = 1;
        }
        if (end_flag >= 0) {
//                    DBG("send bin bytes: %d", n);
            int ret = monitor_send_binary((*fd), filebuf, n);
            if (ret != n+5) {
//                        DBG("network error: %d", ret);
                end_flag = -2;
            }
        }
        if (end_flag != 0) {
            break;
        }
    }
    fclose(f);
    if (end_flag == -1) {
        DBG("dev %s upgrade failed, open file failed3", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 5, 80, "发送读取文件失败");
    } else if (end_flag == -2) {
        DBG("dev %s upgrade failed, net work errror", ip);
		update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 5, 80, "发送文件失败");
    } else {
		update_c500_upgrade_state(ip, C500_UPGRADE_DOING, 5, 80, "发送文件完成");
        ret = monitor_recv_update_result((*fd));
        if (ret == 0) {
            DBG("dev %s upgrade success", ip);
			update_c500_upgrade_state(ip, C500_UPGRADE_SUCCEED, 6, 100, "upgrade success");
        } else {
            DBG("dev %s upgrade failed, dev return: %d", ip, ret);
			update_c500_upgrade_state(ip, C500_UPGRADE_FAILED, 6, 100, "upgrade failed");
        }
    }
    close((*fd)); (*fd) = -1;
	DBG("upgrade c500 over %s", ip);
	return;
}

static void* heatsdk_client_loop(void *arg)
{
    int fd = -1;
    int timeout_cnt = 0;
    const char *pip = (const char *)arg;
    char *ip = strdup(pip);
	int ret = 0;
    while (1) {
        if (ip == NULL || (int)strlen(ip) < 0) {
            break;
        }
        heatsdk_client_t *client = heatsdk_client_search(ip);
//        if (client == NULL || strlen(client->ip) <= 0 || client->msg_fd < 0) {
		if (client == NULL) {
            DBG("heat client %s:5000 already stoped", ip);
            break;
        }
//        int msg_fd = client->msg_fd;
        if (fd < 0) {
            fd = creat_tcp_sock(ip, 5000);
            if (fd < 0) {
                DBG("connecting heat client %s:5000 failed, return: %d", ip, fd);
             /*   if (local_cb) {
                    monitor_client_t client_obj;
                    memset(&client_obj, 0, sizeof(monitor_client_t));
                    local_cb(ip, &client_obj.data, HEATSDK_NET_ERROR);
					store_mcu_info(ip, &client_obj.data, HEATSDK_NET_ERROR);
                }
              */
                	monitor_client_t client_obj;
                    memset(&client_obj, 0, sizeof(monitor_client_t));
                    mcu_callback(ip, &client_obj.data, HEATSDK_NET_ERROR);
					store_mcu_info(ip, &client_obj.data, HEATSDK_NET_ERROR);
				sleep(10);
				continue;
            }
        }
        fd_set fds;
        FD_ZERO(&fds);
        if (fd >= 0) {
            client->conn_ok = 1;
            FD_SET(fd, &fds);
        } else {
            client->conn_ok = -1;
        }
//        FD_SET(msg_fd, &fds);
        int maxfd = fd;
//        if (msg_fd > fd) {
//            maxfd = msg_fd;
//        }
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000*100;
        ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (ret < 0) {
            if (fd >= 0)
                close(fd);
            fd = -1;
            continue;
        } else if (ret == 0) {
            timeout_cnt++;
            if (timeout_cnt > 500) {
                if (fd >= 0)
                    close(fd);
                fd = -1;
                DBG("connection %s:5000 timeout, reconnection fd%d", ip, fd);
			    monitor_client_t client_obj;
                memset(&client_obj, 0, sizeof(monitor_client_t));
                mcu_callback(ip, &client_obj.data, HEATSDK_NET_ERROR);
				store_mcu_info(ip, &client_obj.data, HEATSDK_NET_ERROR);
            }
        //    continue;
        }
#if 0
        if (FD_ISSET(msg_fd, &fds)) {
            char buf[256] = {0};
            recv(msg_fd, buf, sizeof(buf)-1, 0);
            const char *change_ip_cmd = "change_ip";
            const char *upgrade_cmd = "upgrade";
            if (strcmp(buf, "stop") == 0) {
                pthread_mutex_lock(&heatsdk_client_mutex);
                heatsdk_client_close_internal(ip);
                pthread_mutex_unlock(&heatsdk_client_mutex);
                break;
            } else if (strcmp(buf, "heat_start") == 0) {
                sendHeatCmd(fd, 1);
            } else if (strcmp(buf, "heat_stop") == 0) {
                sendHeatCmd(fd, 0);
            } else if (strcmp(buf, "reset_deg") == 0) {
                sendResetCmd(fd);
            } else if (strncmp(buf, change_ip_cmd, strlen(change_ip_cmd)) == 0) {
                char *ip = buf + strlen(change_ip_cmd);
                sendRecvChangeIp(fd, ip);
            } else if (strncmp(buf, upgrade_cmd, strlen(upgrade_cmd)) == 0) {
                char *path = buf + strlen(upgrade_cmd);
                sendRecvUpgrade(&fd, path, ip);
                continue;
            }
        }
#endif

        if (fd >= 0 && FD_ISSET(fd, &fds)) {
            timeout_cnt = 0;
            monitor_client_t client_obj;
            memset(&client_obj, 0, sizeof(monitor_client_t));
            ret = process_monitor_data_client(fd, &client_obj);
            if (ret < 0) {
                DBG("%s process data failed, return: %d, closing connection fd%d", ip, ret, fd);
                close(fd); fd = -1;
                mcu_callback(ip, &client_obj.data, HEATSDK_NET_ERROR);
				continue;
            } else if (ret == 0) {
                DBG("connecton %s:5000 is booting", ip);
				mcu_callback(ip, &client_obj.data, HEATSDK_DEV_BOOTING);
				continue;
            } else if (ret == 2) {
                DBG("connecton %s:5000 is upgrading firmware, closing connections", ip);
                close(fd); fd = -1;
				mcu_callback(ip, &client_obj.data, HEATSDK_DEV_UPGRADE);
                sleep(10);
				continue;
            } else if (ret == 1) {
				mcu_callback(ip, &client_obj.data, HEATSDK_DEV_OK);
			//	client_obj.data.led_lightness = ledGetVal(fd);
				ret = ledGetVal(fd, client_obj.data);
				if (ret == 1)
				{//20200706版本协议，无角度阈值
					client_obj.data.angle_threshold_valueX = 0;
					client_obj.data.angle_threshold_valueY = 0;
					client_obj.data.angle_threshold_valueZ = 0;
					client_obj.data.angle_threshold_value_quake = 0;
				}
				else
				{
					angleThresholdGet(fd, client_obj.data);
				}
                char client_sn_num[64] = {0};
                if(get_store_sn_num(ip, client_sn_num) == 0)
                {
                    if(SNNumGet(fd, client_sn_num) > 0)
                    {
                        store_mcu_sn_num(ip, client_sn_num);
                    }
                }
				store_mcu_info(ip, &client_obj.data, HEATSDK_DEV_OK);
            }
        }
		
		//控制命令
		C500_CONTROL_STRU control_command;
		int control_flag = 0;
		get_controller_command(&control_command);
		if(strcmp(control_command.ip, ip) == 0)
		{
			char err_msg[64] = {0};
			memset(err_msg, 0, 64);
			ret = -1;
			DBG("comm_type=%d, control_code=%s", control_command.comm_type,
					control_command.control_code);
			
			if(strcmp(control_command.control_code, "003") == 0){
                //风扇控制
				ret = sendFanOnOffCmd(fd, control_command.comm_type);
			}else if(strcmp(control_command.control_code, "004") == 0){
                //加热器控制
				ret = sendHeatCmd(fd, control_command.comm_type);
			}else if(strcmp(control_command.control_code, "005") == 0){
                //角度控制
				if(control_command.comm_type == 1){
					ret = sendResetCmd(fd);
				}else if(control_command.comm_type == 2){
					//设置角度报警阈值
					ret = sendAngleThresholdSet(fd, control_command.comm_value);
				}
			}else if(strcmp(control_command.control_code, "006") == 0){
                //补光灯控制
				if((control_command.comm_type == 1) || (control_command.comm_type == 0)){
					ret = sendLedOnOffCmd(fd, control_command.comm_type);
				}else if(control_command.comm_type == 2){
					control_flag = 122;
					//设置补光灯亮度
					int ret1 = ledSetVal(fd, (int)control_command.comm_value[0], (int)control_command.comm_value[1], 
									(int)control_command.comm_value[2]);
					//设置光敏阈值
					int ret2 = 0;
					if((control_command.comm_value[4]>0) && (control_command.comm_value[5]>0))
					{
						int ret2 = ledThresholdSet(fd, (int)control_command.comm_value[4], (int)control_command.comm_value[5]);
					}
					if((ret1==0) && (ret2==0)){
						ret = 0;
					}else{
						ret = -1;
					}
					sprintf(err_msg, "led light_set result %d, threshold_set result %d", ret1, ret2);
				}
			}else if(strcmp(control_command.control_code, "007") == 0){
                //继电器控制
				if(control_command.comm_type == 2){
					ret = sendRebootCmd(fd);
				}
			}
			LockGuard lock_gurad(&c500_control_mutex);
			if(ret == 0)
			{
				g_c500_control_command.comm_result = 1;
			}
			else
			{
				g_c500_control_command.comm_result = 0;
			}
			memset(g_c500_control_command.ip, 0, 32);
			memset(g_c500_control_command.control_code, 0, 16);
			sprintf(g_c500_control_command.comm_msg, "%s", err_msg);
		}
		if(control_flag == 122)
		{
			sleep(1);
			control_flag = 0;
			monitor_data_t tmp_mcu_data;
            memset(&tmp_mcu_data, 0, sizeof(monitor_client_t));
			ledGetVal(fd, tmp_mcu_data);
			store_led_val(ip, &tmp_mcu_data);
		}

		///升级命令
		char up_file_path[128] = {0};
		if(find_c500_upgrade_task(ip, up_file_path) == C500_UPGRADE_WAIT)
		{
			DBG("start upgrade c500 %s", ip);
			sendRecvUpgrade(&fd, up_file_path, ip);
		}
		
    }
    free(ip); ip = NULL;
    if (fd >= 0) {
        close(fd); fd = -1;
    }
    return NULL;
}

int heatsdk_client_start(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip)) { // 同一ip地址不能启动两次
            pthread_mutex_unlock(&heatsdk_client_mutex);
            ERR("err");
            return -1;
        }
    }
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (strlen(g_c500_list[i].ip) <= 0) {
            break;
        }
    }
    if (i >= HEATSDK_MAX_CLIENT) {
        pthread_mutex_unlock(&heatsdk_client_mutex);
        ERR("err");
        return -1;
    }
    snprintf(g_c500_list[i].ip, sizeof(g_c500_list[i].ip)-1, "%s", ip);
#if 0
    int msg_fds[2] = {-1, -1};
    mysocketpair(msg_fds);
    g_c500_list[i].msg_fd     = msg_fds[0];
    g_c500_list[i].msg_cli_fd = msg_fds[1];
#endif
    g_c500_list[i].conn_ok    = -1;
    pthread_mutex_unlock(&heatsdk_client_mutex);
    pthread_t t1;
    int ret = pthread_create(&t1, NULL, heatsdk_client_loop, (void *)ip);
    if (ret == 0) {
        pthread_detach(t1);
    } else {
        pthread_mutex_lock(&heatsdk_client_mutex);
        heatsdk_client_close_internal(ip);
        pthread_mutex_unlock(&heatsdk_client_mutex);
        ERR("err");
        return -1;
    }
    return 0;
}

void heatsdk_client_close(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip)) { // 同一ip地址不能启动两次
            const char *stop_flag = "stop";
            send(g_c500_list[i].msg_cli_fd, stop_flag, strlen(stop_flag)+1, 0);
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
}

int heatsdk_heat_start(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip) && g_c500_list[i].conn_ok) { // 同一ip地址不能启动两次
            const char *flag = "heat_start";
            int ret = send(g_c500_list[i].msg_cli_fd, flag, strlen(flag)+1, 0);
            if (ret != (int)strlen(flag)+1) {
                pthread_mutex_unlock(&heatsdk_client_mutex);
                return HEATSDK_NET_ERROR;
            }
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return HEATSDK_DEV_OK;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return HEATSDK_NET_ERROR;
}

int heatsdk_heat_stop(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip) && g_c500_list[i].conn_ok) {
            const char *flag = "heat_stop";
            int ret = send(g_c500_list[i].msg_cli_fd, flag, strlen(flag)+1, 0);
            if (ret != (int)strlen(flag)+1) {
                pthread_mutex_unlock(&heatsdk_client_mutex);
                return HEATSDK_NET_ERROR;
            }
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return HEATSDK_DEV_OK;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return HEATSDK_NET_ERROR;
}

int heatsdk_reset_deg(const char *ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip) && g_c500_list[i].conn_ok) {
            const char *flag = "reset_deg";
            int ret = send(g_c500_list[i].msg_cli_fd, flag, strlen(flag)+1, 0);
            if (ret != (int)strlen(flag)+1) {
                pthread_mutex_unlock(&heatsdk_client_mutex);
                return HEATSDK_NET_ERROR;
            }
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return HEATSDK_DEV_OK;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return HEATSDK_NET_ERROR;
}

int heatsdk_change_ip(const char *ip, const char *new_ip)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip) && g_c500_list[i].conn_ok) {
            char buf[256] = {0};
            snprintf(buf, sizeof(buf)-1, "change_ip%s", new_ip);
            int ret = send(g_c500_list[i].msg_cli_fd, buf, strlen(buf)+1, 0);
            if (ret != (int)strlen(buf)+1) {
                pthread_mutex_unlock(&heatsdk_client_mutex);
                return HEATSDK_NET_ERROR;
            }
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return HEATSDK_DEV_OK;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return HEATSDK_NET_ERROR;
}

int heatsdk_upgrade(const char *ip, const char *path)
{
    int i;
    pthread_mutex_lock(&heatsdk_client_mutex);
    for (i = 0; i < HEATSDK_MAX_CLIENT; i++) {
        if (inet_addr(ip) == inet_addr(g_c500_list[i].ip) && g_c500_list[i].conn_ok) {
            char buf[256] = {0};
            snprintf(buf, sizeof(buf)-1, "upgrade%s", path);
            int ret = send(g_c500_list[i].msg_cli_fd, buf, strlen(buf)+1, 0);
            if (ret != (int)strlen(buf)+1) {
                pthread_mutex_unlock(&heatsdk_client_mutex);
                return HEATSDK_NET_ERROR;
            }
            pthread_mutex_unlock(&heatsdk_client_mutex);
            return HEATSDK_DEV_OK;
        }
    }
    pthread_mutex_unlock(&heatsdk_client_mutex);
    return HEATSDK_NET_ERROR;
}

void heatsdk_client_callback(monitor_callback_t cb)
{
    local_cb = cb;
}


