//
// Created by lining on 12/26/22.
//

#ifndef _SIGNALCONTROLCOM_H
#define _SIGNALCONTROLCOM_H

#include <stdint.h>
#include <vector>
#include <map>

using namespace std;

vector<uint8_t> TransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet);

vector<uint8_t> DeTransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet);


/**
 * GBT20999-2017标准规定
 * 物理接口为NET(tcp udp)和
 * RS232(字节结构为一个起始位，八个数据位，一个校验位，一个结束位；接口支持比特率至少包括：1200bit/s、2400bit/s、4800bit/s、9600bit/s和19200bit/s)
 * 通信帧结构
 * 协议约定：
 * ａ） 信号机使用RS-232-C、TCP/IP通信方式时需要使用转义字符，当信号机使用UDP/IP通信方 式时，不需要使用转义字符；
 * ｂ） 协议约定开始字节为0x7E，结束字节为0x7D，转义字符为0x5C，在报文数据中，遇到开始字节、结束字节、转义字符，在其前增加转义字符0x5C；
 * ｃ） 转义字符不参与CRC16校验运算；
 * ｄ） 协议约定采用１６进制方式通信，协议数据为多字节时，高字节在前低字节在后；
 * ｅ） 协议约定读取某个属性或者元素的全部值时，属性或者元素字段填0。
 *
 */


//协议大帧,本地是主机的小端字节，通信的时候为大端字节,这里具体字节按实际情况存，在组织发送字节的时候做多字节转换
namespace ComFrame_GBT20999_2017 {

    typedef enum {
        Type_Query = 0x10,//查询
        Type_QueryReply = 0x20,//查询应答
        Type_QueryErrorReply = 0x21,//查询出错回复
        Type_Set = 0x30,//设置
        Type_SetReply = 0x40,//设置应答
        Type_SetErrorReply = 0x41,//设置出错回复
        Type_Broadcast = 0x50,//广播
        Type_Trap = 0x60,//主动上报
        Type_HeartSearch = 0x70,//心跳查询
        Type_HeartReply = 0X80,//心跳应答
    } Type;//这里的规律是，回复方的类型为 成功：发送方的类型+0x10;失败：发送方的类型+0x11


    class DataItem {
    public:
        uint8_t index;//该条数据值在报文所有操作数据值中的索引位置,从1开始
        uint8_t length;//该条数据值在报文中的长度，包含数据类ID、对象ID、属性ID、数据元素ID、数据值的长度
        uint8_t typeID = 0;//信号机内部数据结构以数据类的形式来组织
        uint8_t objID = 0;//对象是数据类中某一个具体的数据内容的定义
        uint8_t attrID = 0;//数据对象的具体属性，在查询类型报文中，当属性值为0时，表示读取该对象的所有属性值
        uint8_t elementID = 0;//具体元素的索引，在查询类型报文中，当元素ID为0时，表示读取该属性的 所有元素值
        vector<uint8_t> data;//具体的某个标识号代表的数据或者该标识号的值状态，帧类型为查询类型 时，数据值长度为0
        int getFromBytes(vector<uint8_t> in);

        int setToBytes(vector<uint8_t> &out);

        //根据类具体的各个元素属性
        void GetGroup(uint8_t index, uint8_t typeID, uint8_t objID, uint8_t attrID, uint8_t elementID,
                      vector<uint8_t> *data = nullptr);
    };//数据值集合

    class FrameAll {
    private:
        bool isUpdateLength = false;
        bool isUpdateCRC = false;
    public:
        uint8_t begin = 0x7e;//开始字节,固定0x7e
        uint16_t length;//报文长度，不包含报文头尾
        uint16_t version;//版本号V1.02  0x0102
        uint8_t controlCenterID;//区分和信号机进行通信的上位机的ID
        uint32_t roadTrafficSignalControllerID;//信号机在整个中心系统中的唯一标识符
        uint8_t roadID;//信号机控制多路口时，通信确认路口序号
        uint8_t sequence;//信号机和上位机之间实时通信时，双方确认通信帧发送的顺序。发送方对 帧流水号进行编码，回复方回复数据采用发送方的流水号值
        uint8_t type;//通信帧的类型,详见 Type
        uint8_t dataItemNum;//该条报文需要操作的数据值的数量，一个报文最大可以操作255个数据值
        vector<DataItem> dataItems;//数据值数组
        uint16_t crc16;//报文数据CRC16校验值，CRC生成多项式x16+x12+x2+1,不包含报文头尾字节
        uint8_t end = 0x7d;//结束字节，固定为0x7d
        int getFromBytes(vector<uint8_t> in);

        int setToBytes(vector<uint8_t> &out);

    public:
        //更新长度信息
        void UpdateFrameLength();

        //计算CRC16值
        void UpdateFrameCRC16();

    private:
        //更新变成字节序列的帧内长度
        static int UpdateByteLength(vector<uint8_t> &bytes);

        //更新变成字节序列的帧内校验值
        static int UpdateByteCRC16(vector<uint8_t> &bytes);

    public:
        //从接收的字节流(包含转义字符)内获取本帧内容
        int getFromBytesWithTF(vector<uint8_t> in, bool isCheckCRC = true);

        //根据本帧内容形成要发送的字节流(包含转义字符)
        int setToBytesWithTF(vector<uint8_t> &out);
    };


    //数据类 uin32_t 0x01020304 0x01数据类ID 0x02对象ID 0x03属性ID 0x04元素ID
    typedef struct {
        uint32_t value;
        string name;
    } DataItemValue;

    typedef enum{
        DataItemType_Unknown = 0,
        //设备信息
        DataItemType_Manufacturer = 1,
        DataItemType_DeviceVersion,
        DataItemType_DeviceNo,
        DataItemType_ManufactureDate,
        DataItemType_ConfigDate,
        //基础信息
        DataItemType_InstallRoad,
        DataItemType_RSCNet4IP,
        DataItemType_RSCNet4Mask,
        DataItemType_RSCNet4Gateway,
        DataItemType_CCNet4IP,
        DataItemType_CCNet4Port,
        DataItemType_CCNet4ComType,
        DataItemType_RSCTimezone,
        DataItemType_RSCID,
        DataItemType_RSCControlRoadNum,
        DataItemType_GPSCLKMark,
        DataItemType_RSCNet6IP,
        DataItemType_RSCNet6Mask,
        DataItemType_RSCNet6Gateway,
        DataItemType_CCNet6IP,
        DataItemType_CCNet6Port,
        DataItemType_CCNet6ComType,
        //灯组信息LG
        DataItemType_LGNum,
        DataItemType_LGConfigNo,
        DataItemType_LGConfigType,
        DataItemType_LGState,
        DataItemType_LGControlNo,
        DataItemType_LGControlShield,
        DataItemType_LGControlForbid,
        //相位信息PS 失去路权过渡灯色LRWTLC 获得路权灯色GRWTLC
        DataItemType_PSNum,
        DataItemType_PSConfigNo,
        DataItemType_PSConfigLG,
        DataItemType_PSConfigLRWTLC1Type,
        DataItemType_PSConfigLRWTLC1Time,
        DataItemType_PSConfigLRWTLC2Type,
        DataItemType_PSConfigLRWTLC2Time,
        DataItemType_PSConfigLRWTLC3Type,
        DataItemType_PSConfigLRWTLC3Time,
        DataItemType_PSConfigGRWTLC1Type,
        DataItemType_PSConfigGRWTLC1Time,
        DataItemType_PSConfigGRWTLC2Type,
        DataItemType_PSConfigGRWTLC2Time,
        DataItemType_PSConfigGRWTLC3Type,
        DataItemType_PSConfigGRWTLC3Time,
        DataItemType_PSConfigOnGRWTLC1Type,
        DataItemType_PSConfigOnGRWTLC1Time,
        DataItemType_PSConfigOnGRWTLC2Type,
        DataItemType_PSConfigOnGRWTLC2Time,
        DataItemType_PSConfigOnGRWTLC3Type,
        DataItemType_PSConfigOnGRWTLC3Time,
        DataItemType_PSConfigOnLRWTLC1Type,
        DataItemType_PSConfigOnLRWTLC1Time,
        DataItemType_PSConfigOnLRWTLC2Type,
        DataItemType_PSConfigOnLRWTLC2Time,
        DataItemType_PSConfigOnLRWTLC3Type,
        DataItemType_PSConfigOnLRWTLC3Time,
        DataItemType_PSConfigMinGTime,
        DataItemType_PSConfigMaxGTime1,
        DataItemType_PSConfigMaxGTime2,
        DataItemType_PSConfigExtendGTime,
        DataItemType_PSConfigNeed,
        DataItemType_PSControlNo,
        DataItemType_PSControlShield,
        DataItemType_PSControlForbid,
        //检测器信息 流量监测TC 占有率采集OC
        DataItemType_DetectorNum,
        DataItemType_DetectorConfigNo,
        DataItemType_DetectorConfigType,
        DataItemType_DetectorConfigTCPeriod,
        DataItemType_DetectorConfigOCPeriod,
        DataItemType_DetectorConfigInstallPos,
        DataItemType_DetectorStateNo,
        DataItemType_DetectorStateCarExist,
        DataItemType_DetectorStateCarSpeed,
        DataItemType_DetectorStateCarType,
        DataItemType_DetectorStateCarPlateNumber,
        DataItemType_DetectorStateLaneQueueLen,
        //相位阶段信息 相位阶段PSS
        DataItemType_PSSConfigStageNum,
        DataItemType_PSSConfigStageNo,
        DataItemType_PSSConfigStagePSS,
        DataItemType_PSSConfigStageDelayOnTime,
        DataItemType_PSSConfigStageAheadOffTime,
        DataItemType_PSSStateNo,
        DataItemType_PSSStateState,
        DataItemType_PSSStateOnTime,
        DataItemType_PSSStateLastTime,
        DataItemType_PSSControlNo,
        DataItemType_PSSControlSoftNeed,
        DataItemType_PSSControlShieldMark,
        DataItemType_PSSControlForbidMark,
        //相位安全信息
        DataItemType_PSClashNo,
        DataItemType_PSClashSequence,
        DataItemType_PSGIntervalNo,
        DataItemType_PSGIntervalSequence,
        //紧急优先URP
        DataItemType_URPPriorNum,
        DataItemType_URPPriorConfigNo,
        DataItemType_URPPriorConfigReqPSS,
        DataItemType_URPPriorConfigReqPri,
        DataItemType_URPPriorConfigShieldMark,
        DataItemType_URPPriorStateNo,
        DataItemType_URPPriorStateReqState,
        DataItemType_URPPriorStateExeState,
        DataItemType_URPUrgentNum,
        DataItemType_URPUrgentConfigNo,
        DataItemType_URPUrgentConfigReqPSS,
        DataItemType_URPUrgentConfigReqPri,
        DataItemType_URPUrgentConfigShieldMark,
        DataItemType_URPUrgentStateNo,
        DataItemType_URPUrgentStateReqState,
        DataItemType_URPUrgentStateExeState,
        //方案信息CP
        DataItemType_CPNum,
        DataItemType_CPConfigNo,
        DataItemType_CPConfigLaneSeq,
        DataItemType_CPConfigPeriod,
        DataItemType_CPConfigCoordinateSeq,
        DataItemType_CPConfigPSOffsetTime,
        DataItemType_CPConfigPSSChain,
        DataItemType_CPConfigPSSTimeChain,
        DataItemType_CPConfigPSSTypeChain,
        //过渡约束PTC
        DataItemType_PTCConfigNo,
        DataItemType_PTCConfigValue,
        //日计划TT
        DataItemType_TTNum,
        DataItemType_TTConfigNo,
        DataItemType_TTConfigLaneSeq,
        DataItemType_TTConfigStartTimeChain,
        DataItemType_TTConfigExePlanChain,
        DataItemType_TTConfigOnModelChain,
        DataItemType_TTConfigActionChain1,
        DataItemType_TTConfigActionChain2,
        DataItemType_TTConfigActionChain3,
        DataItemType_TTConfigActionChain4,
        DataItemType_TTConfigActionChain5,
        DataItemType_TTConfigActionChain6,
        DataItemType_TTConfigActionChain7,
        DataItemType_TTConfigActionChain8,
        //调度表DS
        DataItemType_DSNum,
        DataItemType_DSConfigNo,
        DataItemType_DSConfigLaneSeq,
        DataItemType_DSConfigPrior,
        DataItemType_DSConfigWeek,
        DataItemType_DSConfigMonth,
        DataItemType_DSConfigDay,
        DataItemType_DSConfigTT,
        //运行状态 OnState
        DataItemType_OnStateDeviceDetectorState,
        DataItemType_OnStateDeviceModelState,
        DataItemType_OnStateDeviceGateState,
        DataItemType_OnStateDeviceVoltage,
        DataItemType_OnStateDeviceCurrent,
        DataItemType_OnStateDeviceTemperature,
        DataItemType_OnStateDeviceHumidity,
        DataItemType_OnStateDeviceWater,
        DataItemType_OnStateDeviceSmoke,
        DataItemType_OnStateDeviceRSCUTC,
        DataItemType_OnStateDeviceRSCLocTime,
        DataItemType_OnStateLaneSeq,
        DataItemType_OnStateOnModel,
        DataItemType_OnStateCurPlan,
        DataItemType_OnStateCurPSS,
        //交通数据 TD
        DataItemType_TDLiveData,
        DataItemType_TDStatisticDataDetectorNo,
        DataItemType_TDStatisticDataDetectorFlow,
        DataItemType_TDStatisticDataDetectorOccupy,
        DataItemType_TDStatisticDataAvgSpeed,
        //报警数据 AlarmData
        DataItemType_AlarmDataCurNum,
        DataItemType_AlarmDataNo,
        DataItemType_AlarmDataType,
        DataItemType_AlarmDataValue,
        DataItemType_AlarmDataTime,
        //故障数据 BugData
        DataItemType_BugDataCurNum,
        DataItemType_BugDataNo,
        DataItemType_BugDataType,
        DataItemType_BugDataTime,
        DataItemType_BugDataAction,
        //中心控制CenterControl
        DataItemType_CenterControlLaneID,
        DataItemType_CenterControlAssignPSS,
        DataItemType_CenterControlAssignPlan,
        DataItemType_CenterControlAssignOnModel,
        //命令管道
        DataItemType_CmdPipeValue,
    }DataItemType;

    static map<DataItemType, DataItemValue> DataItemMap = {
            //设备信息
            {DataItemType_Manufacturer,{0x01010000,"制造厂商"}},
            {DataItemType_DeviceVersion,{0x01020000,"设备版本"}},
            {DataItemType_DeviceNo,{0x01030000,"设备编号"}},
            {DataItemType_ManufactureDate,{0x01040000,"出厂日期"}},
            {DataItemType_ConfigDate,{0x01050000,"配置日期"}},
            //基础信息
            {DataItemType_InstallRoad,{0x02010000,"信号机安装路口"}},
            {DataItemType_RSCNet4IP,{0x02020100,"信号机IPV4网络配置IP"}},
            {DataItemType_RSCNet4Mask,{0x02020200,"信号机IPV4网络配置子网掩码"}},
            {DataItemType_RSCNet4Gateway,{0x02020300,"信号机IPV4网络配置网关"}},
            {DataItemType_CCNet4IP,{0x02030100,"上位机IPV4网络配置IP"}},
            {DataItemType_CCNet4Port,{0x02030200,"上位机IPV4网络配置通信端口"}},
            {DataItemType_CCNet4ComType,{0x02030300,"上位机IPV4网络配置通信类型"}},
            {DataItemType_RSCTimezone,{0x02040000,"信号机所属时区"}},
            {DataItemType_RSCID,{0x02050000,"信号机ID"}},
            {DataItemType_RSCControlRoadNum,{0x02060000,"信号机控制路口数量"}},
            {DataItemType_GPSCLKMark,{0x02070000,"GPS时钟标志"}},
            {DataItemType_RSCNet6IP,{0x02080100,"信号机IPV6网络配置IP"}},
            {DataItemType_RSCNet6Mask,{0x02080200,"信号机IPV6网络配置子网掩码"}},
            {DataItemType_RSCNet6Gateway,{0x02080300,"信号机IPV6网络配置网关"}},
            {DataItemType_CCNet6IP,{0x02090100,"上位机IPV6网络配置IP"}},
            {DataItemType_CCNet6Port,{0x02090200,"上位机IPV6网络配置通信端口"}},
            {DataItemType_CCNet6ComType,{0x02090300,"上位机IPV6网络配置通信类型"}},
            //灯组信息
            {DataItemType_LGNum,{0x03010000,"实际灯组数"}},
            {DataItemType_LGConfigNo,{0x03020100,"灯组配置表编号"}},
            {DataItemType_LGConfigType,{0x03020200,"灯组配置表类型"}},
            {DataItemType_LGState,{0x03030100,"灯组状态"}},
            {DataItemType_LGControlNo,{0x03040100,"灯组控制表编号"}},
            {DataItemType_LGControlShield,{0x03040200,"灯组控制表屏蔽"}},
            {DataItemType_LGControlForbid,{0x03040300,"灯组控制表禁止"}},
            //相位信息
            {DataItemType_PSNum,{0x04010000,"实际相位数"}},
            {DataItemType_PSConfigNo,{0x04020100,"相位配置表编号"}},
            {DataItemType_PSConfigLG,{0x04020200,"相位配置表灯组"}},
            {DataItemType_PSConfigLRWTLC1Type,{0x04020300,"相位配置表失去路权过渡灯色1类型"}},
            {DataItemType_PSConfigLRWTLC1Time,{0x04020400,"相位配置表失去路权过渡灯色1时间"}},
            {DataItemType_PSConfigLRWTLC2Type,{0x04020500,"相位配置表失去路权过渡灯色2类型"}},
            {DataItemType_PSConfigLRWTLC2Time,{0x04020600,"相位配置表失去路权过渡灯色2时间"}},
            {DataItemType_PSConfigLRWTLC3Type,{0x04020700,"相位配置表失去路权过渡灯色3类型"}},
            {DataItemType_PSConfigLRWTLC3Time,{0x04020800,"相位配置表失去路权过渡灯色3时间"}},
            {DataItemType_PSConfigGRWTLC1Type,{0x04020900,"相位配置表获得路权过渡灯色1类型"}},
            {DataItemType_PSConfigGRWTLC1Time,{0x04020a00,"相位配置表获得路权过渡灯色1时间"}},
            {DataItemType_PSConfigGRWTLC2Type,{0x04020b00,"相位配置表获得路权过渡灯色2类型"}},
            {DataItemType_PSConfigGRWTLC2Time,{0x04020c00,"相位配置表获得路权过渡灯色2时间"}},
            {DataItemType_PSConfigGRWTLC3Type,{0x04020d00,"相位配置表获得路权过渡灯色3类型"}},
            {DataItemType_PSConfigGRWTLC3Time,{0x04020e00,"相位配置表获得路权过渡灯色3时间"}},
            {DataItemType_PSConfigOnGRWTLC1Type,{0x04020f00,"相位配置表开机获得路权过渡灯色1类型"}},
            {DataItemType_PSConfigOnGRWTLC1Time,{0x04021000,"相位配置表开机获得路权过渡灯色1时间"}},
            {DataItemType_PSConfigOnGRWTLC2Type,{0x04021100,"相位配置表开机获得路权过渡灯色2类型"}},
            {DataItemType_PSConfigOnGRWTLC2Time,{0x04021200,"相位配置表开机获得路权过渡灯色2时间"}},
            {DataItemType_PSConfigOnGRWTLC3Type,{0x04021300,"相位配置表开机获得路权过渡灯色3类型"}},
            {DataItemType_PSConfigOnGRWTLC3Time,{0x04021400,"相位配置表开机获得路权过渡灯色3时间"}},
            {DataItemType_PSConfigOnLRWTLC1Type,{0x04021500,"相位配置表开机失去路权过渡灯色1类型"}},
            {DataItemType_PSConfigOnLRWTLC1Time,{0x04021600,"相位配置表开机失去路权过渡灯色1时间"}},
            {DataItemType_PSConfigOnLRWTLC2Type,{0x04021700,"相位配置表开机失去路权过渡灯色2类型"}},
            {DataItemType_PSConfigOnLRWTLC2Time,{0x04021800,"相位配置表开机失去路权过渡灯色2时间"}},
            {DataItemType_PSConfigOnLRWTLC3Type,{0x04021900,"相位配置表开机失去路权过渡灯色3类型"}},
            {DataItemType_PSConfigOnLRWTLC3Time,{0x04021a00,"相位配置表开机失去路权过渡灯色3时间"}},
            {DataItemType_PSConfigMinGTime,{0x04021b00,"相位配置表最小绿时间"}},
            {DataItemType_PSConfigMaxGTime1,{0x04021c00,"相位配置表最大绿时间1"}},
            {DataItemType_PSConfigMaxGTime2,{0x04021d00,"相位配置表最大绿时间2"}},
            {DataItemType_PSConfigExtendGTime,{0x04021e00,"相位配置表延长绿时间"}},
            {DataItemType_PSConfigNeed,{0x04021f00,"相位配置表需求"}},
            {DataItemType_PSControlNo,{0x04030100,"相位控制表编号"}},
            {DataItemType_PSControlShield,{0x04030200,"相位控制表屏蔽"}},
            {DataItemType_PSControlForbid,{0x04030300,"相位控制表禁止"}},
            //检测器信息
            {DataItemType_DetectorNum,{0x05010000,"实际检测器数"}},
            {DataItemType_DetectorConfigNo,{0x05020100,"检测器配置表编号"}},
            {DataItemType_DetectorConfigType,{0x05020200,"检测器配置表类型"}},
            {DataItemType_DetectorConfigTCPeriod,{0x05020300,"检测器配置表流量采集周期"}},
            {DataItemType_DetectorConfigOCPeriod,{0x05020400,"检测器配置表占有率采集周期"}},
            {DataItemType_DetectorConfigInstallPos,{0x05020500,"检测器配置表安装位置"}},
            {DataItemType_DetectorStateNo,{0x05030100,"检测器状态表编号"}},
            {DataItemType_DetectorStateCarExist,{0x05030200,"检测器状态表车辆存在状态"}},
            {DataItemType_DetectorStateCarSpeed,{0x05030300,"检测器状态表车辆速度"}},
            {DataItemType_DetectorStateCarType,{0x05030400,"检测器状态表车辆类型"}},
            {DataItemType_DetectorStateCarPlateNumber,{0x05030500,"检测器状态表车辆号牌"}},
            {DataItemType_DetectorStateLaneQueueLen,{0x05030600,"检测器状态表所在车道排队长度"}},
            //相位阶段信息
            {DataItemType_PSSConfigStageNum,{0x06010000,"实际配置相位阶段数"}},
            {DataItemType_PSSConfigStageNo,{0x06020100,"相位阶段配置表编号"}},
            {DataItemType_PSSConfigStagePSS,{0x06020200,"相位阶段配置表相位"}},
            {DataItemType_PSSConfigStageDelayOnTime,{0x06020300,"相位阶段配置表相位晚起动时间"}},
            {DataItemType_PSSConfigStageAheadOffTime,{0x06020400,"相位阶段配置表相位早结束时间"}},
            {DataItemType_PSSStateNo,{0x06030100,"相位阶段状态表编号"}},
            {DataItemType_PSSStateState,{0x06030200,"相位阶段状态表状态"}},
            {DataItemType_PSSStateOnTime,{0x06030300,"相位阶段状态表已经运行时间"}},
            {DataItemType_PSSStateLastTime,{0x06030400,"相位阶段状态表剩余时间"}},
            {DataItemType_PSSControlNo,{0x06040100,"相位阶段控制表编号"}},
            {DataItemType_PSSControlSoftNeed,{0x06040200,"相位阶段控制表软件需求"}},
            {DataItemType_PSSControlShieldMark,{0x06040300,"相位阶段控制表屏蔽标志"}},
            {DataItemType_PSSControlForbidMark,{0x06040400,"相位阶段控制表禁止标志"}},
            //相位安全信息
            {DataItemType_PSClashNo,{0x07010100,"相位冲突配置表编号"}},
            {DataItemType_PSClashSequence,{0x07010200,"相位冲突配置表序列"}},
            {DataItemType_PSGIntervalNo,{0x07020100,"相位绿间隔配置表编号"}},
            {DataItemType_PSGIntervalSequence,{0x07020200,"相位绿间隔配置表序列"}},
            //紧急优先
            {DataItemType_URPPriorNum,{0x08010000,"紧急优先实际优先数量"}},
            {DataItemType_URPPriorConfigNo,{0x08020100,"紧急优先优先配置表编号"}},
            {DataItemType_URPPriorConfigReqPSS,{0x08020200,"紧急优先优先配置表申请相位阶段"}},
            {DataItemType_URPPriorConfigReqPri,{0x08020300,"紧急优先优先配置表申请优先级"}},
            {DataItemType_URPPriorConfigShieldMark,{0x08020400,"紧急优先优先配置表屏蔽标志"}},
            {DataItemType_URPPriorStateNo,{0x08030100,"紧急优先优先状态表编号"}},
            {DataItemType_URPPriorStateReqState,{0x08030200,"紧急优先优先状态表申请状态"}},
            {DataItemType_URPPriorStateExeState,{0x08030300,"紧急优先优先状态表执行状态"}},
            {DataItemType_URPUrgentNum,{0x08040000,"紧急优先实际紧急数量"}},
            {DataItemType_URPUrgentConfigNo,{0x08050100,"紧急优先紧急配置表编号"}},
            {DataItemType_URPUrgentConfigReqPSS,{0x08050200,"紧急优先紧急配置表申请相位阶段"}},
            {DataItemType_URPUrgentConfigReqPri,{0x08050300,"紧急优先紧急配置表申请优先级"}},
            {DataItemType_URPUrgentConfigShieldMark,{0x08050400,"紧急优先紧急配置表屏蔽标志"}},
            {DataItemType_URPUrgentStateNo,{0x08060100,"紧急优先紧急状态表编号"}},
            {DataItemType_URPUrgentStateReqState,{0x08060200,"紧急优先紧急状态表申请状态"}},
            {DataItemType_URPUrgentStateExeState,{0x08060300,"紧急优先紧急状态表执行状态"}},
            //方案信息
            {DataItemType_CPNum,{0x09010000,"方案信息实际方案数"}},
            {DataItemType_CPConfigNo,{0x09020100,"方案信息方案配置表编号"}},
            {DataItemType_CPConfigLaneSeq,{0x09020200,"方案信息方案配置表所属路口序号"}},
            {DataItemType_CPConfigPeriod,{0x09020300,"方案信息方案配置表周期"}},
            {DataItemType_CPConfigCoordinateSeq,{0x09020400,"方案信息方案配置表协调序号"}},
            {DataItemType_CPConfigPSOffsetTime,{0x09020500,"方案信息方案配置表相位差时间"}},
            {DataItemType_CPConfigPSSChain,{0x09020600,"方案信息方案配置表相位阶段链"}},
            {DataItemType_CPConfigPSSTimeChain,{0x09020700,"方案信息方案配置表相位阶段时间链"}},
            {DataItemType_CPConfigPSSTypeChain,{0x09020800,"方案信息方案配置表相位阶段出现类型链"}},
            //过渡约束
            {DataItemType_PTCConfigNo,{0x0a010100,"过渡约束相位阶段过渡约束配置表编号"}},
            {DataItemType_PTCConfigValue,{0x0a010200,"过渡约束相位阶段过渡约束配置表约束值"}},
            //日计划
            {DataItemType_TTNum,{0x0b010000,"日计划实际日计划数量"}},
            {DataItemType_TTConfigNo,{0x0b020100,"日计划配置编号"}},
            {DataItemType_TTConfigLaneSeq,{0x0b020200,"日计划配置所属路口序号"}},
            {DataItemType_TTConfigStartTimeChain,{0x0b020300,"日计划时段开始时间链"}},
            {DataItemType_TTConfigExePlanChain,{0x0b020400,"日计划时段执行方案链"}},
            {DataItemType_TTConfigOnModelChain,{0x0b020500,"日计划时段运行模式链"}},
            {DataItemType_TTConfigActionChain1,{0x0b020600,"日计划时段时段动作链1"}},
            {DataItemType_TTConfigActionChain2,{0x0b020700,"日计划时段时段动作链2"}},
            {DataItemType_TTConfigActionChain3,{0x0b020800,"日计划时段时段动作链3"}},
            {DataItemType_TTConfigActionChain4,{0x0b020900,"日计划时段时段动作链4"}},
            {DataItemType_TTConfigActionChain5,{0x0b020a00,"日计划时段时段动作链5"}},
            {DataItemType_TTConfigActionChain6,{0x0b020b00,"日计划时段时段动作链6"}},
            {DataItemType_TTConfigActionChain7,{0x0b020c00,"日计划时段时段动作链7"}},
            {DataItemType_TTConfigActionChain8,{0x0b020d00,"日计划时段时段动作链8"}},
            //调度表
            {DataItemType_DSNum,{0x0c010000,"调度表实际调度表数量"}},
            {DataItemType_DSConfigNo,{0x0c020100,"调度表配置编号"}},
            {DataItemType_DSConfigLaneSeq,{0x0c020200,"调度表配置所属路口序号"}},
            {DataItemType_DSConfigPrior,{0x0c020300,"调度表配置优先级"}},
            {DataItemType_DSConfigWeek,{0x0c020400,"调度表配置星期值"}},
            {DataItemType_DSConfigMonth,{0x0c020500,"调度表配置月份值"}},
            {DataItemType_DSConfigDay,{0x0c020600,"调度表配置日期值"}},
            {DataItemType_DSConfigTT,{0x0c020700,"调度表配置日计划"}},
            //运行状态
            {DataItemType_OnStateDeviceDetectorState,{0x0d010100,"运行状态检测器状态"}},
            {DataItemType_OnStateDeviceModelState,{0x0d010200,"运行状态设备模块状态"}},
            {DataItemType_OnStateDeviceGateState,{0x0d010300,"运行状态机柜门状态"}},
            {DataItemType_OnStateDeviceVoltage,{0x0d010400,"运行状态电压"}},
            {DataItemType_OnStateDeviceCurrent,{0x0d010500,"运行状态电流"}},
            {DataItemType_OnStateDeviceTemperature,{0x0d010600,"运行状态温度"}},
            {DataItemType_OnStateDeviceHumidity,{0x0d010700,"运行状态湿度"}},
            {DataItemType_OnStateDeviceWater,{0x0d010800,"运行状态水浸"}},
            {DataItemType_OnStateDeviceSmoke,{0x0d010900,"运行状态烟雾"}},
            {DataItemType_OnStateDeviceRSCUTC,{0x0d010a00,"运行状态信号机标准时"}},
            {DataItemType_OnStateDeviceRSCLocTime,{0x0d010b00,"运行状态信号机本地时间"}},
            {DataItemType_OnStateLaneSeq,{0x0d020100,"运行状态表路口序号"}},
            {DataItemType_OnStateOnModel,{0x0d020200,"运行状态表路口运行模式"}},
            {DataItemType_OnStateCurPlan,{0x0d020300,"运行状态表路口当前方案"}},
            {DataItemType_OnStateCurPSS,{0x0d020400,"运行状态表路口当前相位阶段"}},
            //交通数据
            {DataItemType_TDLiveData,{0x0e010000,"交通数据实时数据"}},
            {DataItemType_TDStatisticDataDetectorNo,{0x0e020100,"交通数据统计数据表检测器编号"}},
            {DataItemType_TDStatisticDataDetectorFlow,{0x0e020200,"交通数据统计数据表检测器流量"}},
            {DataItemType_TDStatisticDataDetectorOccupy,{0x0e020300,"交通数据统计数据表检测器占有率"}},
            {DataItemType_TDStatisticDataAvgSpeed,{0x0e020400,"交通数据统计数据表平均车速"}},
            //报警数据
            {DataItemType_AlarmDataCurNum,{0x0f010000,"报警数据当前报警数量"}},
            {DataItemType_AlarmDataNo,{0x0f020100,"报警数据表编号"}},
            {DataItemType_AlarmDataType,{0x0f020200,"报警数据表类型"}},
            {DataItemType_AlarmDataValue,{0x0f020300,"报警数据表值"}},
            {DataItemType_AlarmDataTime,{0x0f020400,"报警数据表时间"}},
            //故障数据
            {DataItemType_BugDataCurNum,{0x10010000,"故障数据当前故障记录数"}},
            {DataItemType_BugDataNo,{0x10020100,"故障数据记录表编号"}},
            {DataItemType_BugDataType,{0x10020200,"故障数据记录表类型"}},
            {DataItemType_BugDataTime,{0x10020300,"故障数据记录表事件发生时间"}},
            {DataItemType_BugDataAction,{0x10020400,"故障数据记录表动作参数"}},
            //中心控制
            {DataItemType_CenterControlLaneID,{0x11010100,"中心控制表路口ID"}},
            {DataItemType_CenterControlAssignPSS,{0x11020000,"中心控制指定相位阶段"}},
            {DataItemType_CenterControlAssignPlan,{0x11030000,"中心控制指定方案"}},
            {DataItemType_CenterControlAssignOnModel,{0x11040000,"中心控制指定运行模式"}},
            //命令管道
            {DataItemType_CmdPipeValue,{0x12010000,"命令管道命令值"}},

    };

    static int ReqGet(DataItem &dataItem, uint8_t index, DataItemType type);

    static int ReqSet(DataItem &dataItem, uint8_t index, DataItemType type, vector<uint8_t> data);

};


#endif //_SIGNALCONTROLCOM_H
