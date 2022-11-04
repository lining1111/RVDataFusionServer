#ifndef C500SDK_H
#define C500SDK_H

#include <pthread.h>


typedef enum{
	C500_NO_UPGRADE_TASK = -1,
	C500_UPGRADE_WAIT = 0,		//待升级
    C500_UPGRADE_DOING,			//升级中
    C500_UPGRADE_SUCCEED,		//升级成功
    C500_UPGRADE_FAILED,			//升级失败
}C500_UPGRADE_STATE;

//升级任务
typedef struct {
    char ip[32];
	char file_path[128];  //升级文件
	char current_software_version[16];
	char target_software_version[16];
	C500_UPGRADE_STATE state;		//升级状态
	int step_total;		//1添加矩阵控制器，2发送升级命令，3进入boot模式，
						//4发送文件大小，5发送文件，6升级成功
	int current_step;
	int progress;
	char message[64];	//处理消息
} C500_UPGRADE_STRU;

//控制命令
typedef struct {
    char control_code[16];    //控制业务代码
    char camera_controller_guid[64]; //矩阵控制器guid
    char ip[32];
	int comm_type;	//控制类型
	int comm_add;	//附加控制命令
	double comm_value[8];	//控制值
	int comm_result;	//处理结果 -1待/正处理，0失败，1成功，
	char comm_msg[64];	//处理消息
//	pthread_mutex_t c500_control_mutex;
} C500_CONTROL_STRU;

//补光灯信息

typedef struct {
	int  led_index;    	//第n路补光灯
	int  led_lightness;	//补光灯亮度值
	char led_state;		//补光灯开关状态，0关闭，1开启 
} led_state_t;
typedef struct {
	int  led_type;		//补光灯型号，55单组，33三组
	int  photosensitive_on_value; 	//光敏开启阈值
	int  photosensitive_off_value;	//光敏关闭阈值
	char led_online_state; //补光灯485连接状态，0错误，1正常

	std::vector<led_state_t> led_state_data;
} led_info_t;

// 读取到的数据

typedef struct {
	float roll;     // 翻滚角度z
	float pitch;    // 俯仰角度y
	float yaw;      // 方位角度x

	float temperature;     // 温度
	float humidity; // 湿度
	float voltage;  // 电压

	char angle_warn;  // 角度报警，1报警，0不报警
	char motion_warn; // 运动报警，1报警，0不报警
	char gyrochip_inited;  // 陀螺仪芯片是否检测到
	char gryo_inited;      // 陀螺仪是否初始
	char heat_state;      // 加热状态，0未加热，1加热
	char fan1_state;	//风扇1工作状态，0停止，1启动
	char fan2_state;	//风扇2工作状态，0停止，1启动
	char led_state;		//补光灯开关状态，0关闭，1开启
	char led_online_state; //补光灯485连接状态，0错误，1正常
	char fan1_fault_state; //风扇1故障状态，1报警，0正常
	char fan2_fault_state; //风扇2故障状态，1报警，0正常

	float angle_threshold_valueX;	//姿态角度方位阈值
	float angle_threshold_valueY;	//姿态角度俯仰阈值
	float angle_threshold_valueZ;	//姿态角度翻滚阈值
	float angle_threshold_value_quake;	//震动阈值
	int  led_lightness;	//补光灯亮度值
	long long data_timestamp; // 数据时间戳，单位毫秒
	int  version;
	led_info_t led_data;

	pthread_mutex_t data_mutex;
} monitor_data_t;

/*
**添加mcu上传矩阵信息
*/
typedef struct {
//    char camera_matrix_id[16];    //相机矩阵编码,编号
    char camera_controller_guid[64]; //矩阵控制器guid
    char camera_controller_name[64]; //矩阵控制器name
    char ip[64];
	int status;		//连接状态 设备未连接-1  ,设备正在启动0,设备正常1,设备正在升级2
	int info_warn_chang;    //mcu告警状态是否改变，有改变1,无改变0
    monitor_data_t data;    //接收到的mcu信息

    char sn_num[64];    //sn编号
//	pthread_mutex_t mcu_info_mutex;
} MCU_DEV_INFO_STRU;


// 设备状态
#define HEATSDK_DEV_BOOTING      0   // 设备正在启动
#define HEATSDK_DEV_OK           1   // 设备正常
#define HEATSDK_DEV_UPGRADE      2   // 设备正在升级
#define HEATSDK_NET_ERROR        -1  // 设备未连接

// 设备读取数据回调函数
// 参数:
//     ip: 设备ip地址
//     data: 设备返回的数据
//     status: 设备状态, 包括HEATSDK_DEV_OK, HEATSDK_DEV_BOOTING, HEATSDK_DEV_UPGRADE, HEATSDK_NET_ERROR
typedef void (*monitor_callback_t)(const char *ip, monitor_data_t *data, int status);

// 设置设备读数回调函数
void heatsdk_client_callback(monitor_callback_t cb);

// 启动设备读取数据后台线程
int  heatsdk_client_start(const char *ip);


///////////以下函数需要先调用heatsdk_client_start，连接成功后才能调用///////////
// 启动加热
// 返回值:
//       成功返回HEATSDK_DEV_OK, 失败返回负数
int heatsdk_heat_start(const char *ip);
// 停止加热
int heatsdk_heat_stop(const char *ip);

// 重置角度值
// 返回值:
//       成功返回HEATSDK_DEV_OK, 失败返回负数
int heatsdk_reset_deg(const char *ip);

// 修改设备ip地址
// 参数:
//      new_ip: 新的ip地址
// 返回值:
//       成功返回HEATSDK_DEV_OK, 失败返回负数
int heatsdk_change_ip(const char *ip, const char *new_ip);

// 升级设备固件
// 参数:
//      path: 新的固件路径
// 返回值:
//       成功返回HEATSDK_DEV_OK, 失败返回负数
int heatsdk_upgrade(const char *ip, const char *path);

// 关闭设备连接
void heatsdk_client_close(const char *ip);

//初始化mcu设备ip
int mcu_dev_init(void);

//启动c500通信
int start_c500_comm(void);

//取mcu_dev_data的个数
size_t get_mcu_device_size(void);

/*取矩阵控制器告警状态是否变化，返回值 1，告警状态改变；2，没改变*/
int get_mcu_warn_change_flag(void);

/*矩阵控制器告警状态标志清0*/
int clean_mcu_warn_change_flag(void);

/*取矩阵控制器状态信息*/
int get_mcu_dev_status_info(std::vector<MCU_DEV_INFO_STRU> &data);

/*设置g_c500_control_command控制命令*/
int set_controller_command(C500_CONTROL_STRU *control_command);

/*取c500控制命令处理结果*/
int get_controller_process_result(void);
int get_controller_process_result_msg(char *msg);
//添加c500升级任务，返回值：1此任务已存在，0添加成功
int add_c500_upgrade_task(const char *ip, const char *bin_path, 
		const char* current_software_version, const char* target_software_version);

//获取c500升级状态  。返回值：0正常放回，-1没找到此升级任务
int get_c500_upgrade_state(const char* ip, C500_UPGRADE_STATE* state,
		int* step_total, int* current_step, int* progress, char* message);
//删除c500升级任务，返回值：1删除成功
int del_c500_upgrade_task(const char *ip);


#endif // C500SDK_H

