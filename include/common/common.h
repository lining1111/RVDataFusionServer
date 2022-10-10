//
// Created by lining on 2/16/22.
//

#ifndef _COMMON_H
#define _COMMON_H

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

namespace common {

#define ARRAY_SIZE(x) \
    (sizeof(x)/sizeof(x[0]))
#define OFFSET(type, member)      \
    ( (size_t)( &( ((type*)0)->member)  ) )
#define MEMBER_SIZE(type, member) \
    sizeof(((type *)0)->member)

    /**
     * 整体通信大帧结构
     *  1byte   1byte   1byte       2bytes  4bytes  4bytes  Nbytes      2bytes
     *  帧头      版本  命令标识符       帧号  设备编号    帧长  json格式正文    校验码
     */


    enum CmdType {
        Response = 0x00,//应答指令
        Login = 0x01,//设备登录
        HeartBeat = 0x02,//心跳
        DeviceData = 0x03,//监控数据回传
        DeviceAlarm = 0x04,//设备报警数据
        DeviceStatus = 0x05,//设备状态数据
        DevicePicData = 0x06,//设备视频数据回传
        DeviceMultiView = 0x07,//多目数据回传
    };//命令字类型


#pragma pack(1)
    typedef struct {
        uint8_t tag = '$';//固定的头开始 ‘$’ 0x24
        uint8_t version;//版本号 1.0 hex
        uint8_t cmd;//命令字类型 详见CmdType
        uint16_t sn = 0;//帧号
        uint32_t deviceNO;//设备编号
        uint32_t len = 0;//整包长度，从包头到最后的校验位 <帧头>sizeof(PkgHead)+<正文>(json)+<校验>sizeof(PkgCRC)
    } PkgHead;//包头

    typedef struct {
        uint16_t data;//校验值，从帧头开始到正文的最后一个字节
    } PkgCRC;
#pragma pack()

    //方法名枚举


    //一帧数据格式 <帧头>PkgHead+<正文>string(json)+<校验>PkgCRC
    typedef struct {
        PkgHead head;
        string body;
        PkgCRC crc;
    } Pkg;


    //PkgBody.methodParam.param一般都是json数据，以下是json字符串原始的结构体
    //现在仅有一个监控数据上传业务 Method(WatchData)

    enum State {
        //TODO
    };

    typedef struct {
        int state;// `json "state"`
        string desc;// `json "desc"`
        string value;// `json "value"`
    } Reply;//回复帧

    typedef struct {
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timestamp;// `json "timstamp"` 自1970.1.1 00:00:00到当前的秒数 date +%s获取秒数 date -d @秒数获取时间格式
    } Beats;//心跳帧 "Beats"

    typedef struct {
        int LightID;//`json "LightID"`
        string Light;//`json "Light"`
        int RT;//`json "RT"`
    } AnnuciatorInfo;//信号机属性

    typedef struct {
        int objID = 0;//目标ID
        int objCameraID = 0;//摄像头目标ID
        int objType = 0;//目标类型
        string plates;//车牌号
        string plateColor;//车牌颜色
        int left = 0;//坐标 左
        int top = 0;//坐标 上
        int right = 0;// 坐标 右
        int bottom = 0;//坐标 下
        double locationX = 0.0;
        double locationY = 0.0;
        string distance;//距离
        double directionAngle;//航角度
//        string speed;//速度
        double speedX;
        double speedY;
        double longitude;//经度
        double latitude;//维度
    } ObjTarget;//目标属性

    enum Direction {
        Unknown = 0,//未知
        East = 1,//东
        South = 2,//南
        West = 3,//西
        North = 4,//北
    };

    typedef struct {
        string oprNum;// `json "oprNum"` uuid()
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timstamp;//`json "timstamp"` 自1970.1.1 00:00:00到当前的毫秒数
        string matrixNo;// `json "matrixNo"` 矩阵编号
        string cameraIp;// `json "cameraIp"` 相机编号
        double RecordDateTime;//`json "RecordDateTime"` 抓拍时间
        int isHasImage;//`json "isHasImage"` 是否包含图像
        string imageData;//`json "imageData"` 当前的视频图像数据
        int direction;//方向，枚举类型 enum Direction
        vector<AnnuciatorInfo> listAnnuciatorInfo;//`json "AnnuciatorInfo"` 信号机列表
        vector<ObjTarget> lstObjTarget;//`json "lstObjTarget"` 目标分类
    } WatchData;//监控数据,对应命令字DeviceData


    typedef struct {
        int wayNo;//对应 Direction
        int roID;//雷达目标编号
        int voID;//视频目标编号
    } RvWayObject;

    typedef struct {
        int cameraObjID;
        int left = 0;//坐标 左
        int top = 0;//坐标 上
        int right = 0;// 坐标 右
        int bottom = 0;//坐标 下
    } VideoTargets;

    typedef struct {
        string rvHardCode;
        vector<VideoTargets> lstVideoTargets;
        string imageData;//图像数据
    } VideoData;

    typedef struct {
        int objID = 0;//目标ID
//        int cameraObjID = 0;//图像目标ID
        vector<RvWayObject> listRvWayObject;
        int objType = 0;//目标类型
        int objColor = 0;//目标颜色
        string plates;//车牌号
        string plateColor;//车牌颜色
//        int left = 0;//坐标 左
//        int top = 0;//坐标 上
//        int right = 0;// 坐标 右
//        int bottom = 0;//坐标 下
        float distance;//距离
        float angle;//航角度
        float speed;//速度
        double locationX = 0.0;//平面X位置
        double locationY = 0.0;//平面Y位置
        double longitude = 0.0;//经度
        double latitude = 0.0;//维度
        int flagNew = 0;//
    } ObjMix;//融合后的目标数据

    typedef struct {
        string oprNum;// `json "oprNum"`
        double timstamp;// `json "timstamp"`自1970.1.1 00:00:00到当前的毫秒数
        string crossID;// ``json "crossID"路口编号
        int isHasImage;//`json "isHasImage"` 是否包含图像
//        string imageData;// `json "imageData"` 当前的视频图像数据
        vector<ObjMix> lstObjTarget;// `json "lstObjTarget"`目标分类
        vector<VideoData> lstVideos;//图像数据，包括图像对应的设备编号、图像识别列表、图像base编码
    } FusionData;//多路融合数据,对应命令字DeviceData

    /**
     * 打印hex输出
     * @param data
     * @param len
     */
    void PrintHex(uint8_t *data, uint32_t len);

    /**
     * base64加密
     * @param input 待加密数据
     * @param input_length 待加密数据长度
     * @param output 加密数据
     * @param output_length 加密数据长度
     */
    void
    base64_encode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length);

    /**
     * base64解密
     * @param input 加密数据
     * @param input_length 加密数据长度
     * @param output 解密数据
     * @param output_length 解密数据长度
     */
    void
    base64_decode(unsigned char *input, unsigned int input_length, unsigned char *output, unsigned int *output_length);


    /**
     * 产生uuid
     * @return uuid的string
     */
    string random_uuid(string seed = "89ab");

    //基本解包组包函数
    /**
     * 组包函数,此时pkg内 校验码不对，要在组包的时候更新校验码
     * @param pkg 帧数据
     * @param out 组帧后的数据地址
     * @param len 组帧后的数据长度
     * @return 0：success -1：fail
     */
    int Pack(Pkg &pkg, uint8_t *out, uint32_t *len);

    /**
     * 解包数据
     * @param in 一帧数据包
     * @param len 一阵数据包长度
     * @param pkg 解包后的数据帧
     * @return 0：success -1：fail
     */
    int Unpack(uint8_t *in, uint32_t len, Pkg &pkg);

    /**
     * WatchData 组json
     * @param watchData in WatchData数据输入
     * @param out out json输出
     * @return 0：success -1：fail
     */
    int JsonMarshalWatchData(WatchData watchData, string &out);

    /**
     * WatchData 解json
     * @param in  in json字符串
     * @param watchData out WatchData数据输出
     * @return 0：success -1：fail
     */
    int JsonUnmarshalWatchData(string in, WatchData &watchData);

    /**
     * 组包监控数据
     * @param watchData in 监控数据结构体
     * @param sn 帧号
     * @param deviceNO 设备编号
     * @param pkg out 包结构体
     * @return 0:success -1:fail
     */
    int PkgWatchDataWithoutCRC(WatchData watchData, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    /**
     * FusionData 组json
     * @param fusionData in FusionData数据输入
     * @param out out json字符串输出
     * @return 0：success -1：fail
     */
    int JsonMarshalFusionData(FusionData fusionData, string &out);

    /**
     * FusionData 解json
     * @param in in json字符串输入
     * @param fusionData out FusionData结构体输出
     * @return 0：success -1：fail
     */
    int JsonUnmarshalFusionData(string in, FusionData &fusionData);

    /**
     * 组包融合后的数据
     * @param fusionData 融合后数据结构体
     * @param sn 帧号
     * @param deviceNO 设备编号
     * @param pkg out 包结构体
     * @return 0：success -1：fail
     */
    int PkgFusionDataWithoutCRC(FusionData fusionData, uint16_t sn, uint32_t deviceNO, Pkg &pkg);


    //多目数据
    typedef struct {
        string laneCode;
        int laneDirection;//车道方向
        int flowDirection;
        int inCars;
        double inAverageSpeed;//入口平均速度
        int outCars;
        double outAverageSpeed;//出口平均速度
        int queueLen;
        int queueCars;
    } FlowData;

    typedef struct {
        string oprNum;// `json "oprNum"`
        string hardCode;
        double timstamp;// `json "timstamp"`自1970.1.1 00:00:00到当前的毫秒数
        string crossCode;
        vector<FlowData> flowData;
    } TrafficFlow;

    typedef struct {
        string hardCode;
        string crossCode;
        vector<FlowData> flowData;
    } OneRoadTrafficFlow;

    typedef struct {
        string oprNum;// `json "oprNum"`
        string crossID;
        double timestamp;// `json "timestamp"`自1970.1.1 00:00:00到当前的毫秒数
        string recordDateTime;
//        vector<OneRoadTrafficFlow> trafficFlow;
        vector<FlowData> trafficFlow;
    } TrafficFlows;

    int JsonMarshalTrafficFlow(TrafficFlow trafficFlow, string &out);

    int JsonUnmarshalTrafficFlow(string in, TrafficFlow &trafficFlow);

    int JsonMarshalTrafficFlows(TrafficFlows trafficFlows, string &out);

    int JsonUnmarshalTrafficFlows(string in, TrafficFlows &trafficFlows);

    int PkgTrafficFlowsWithoutCRC(TrafficFlows trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg);
}


#endif //_COMMON_H
