//
// Created by lining on 2/16/22.
//

#ifndef _COMMON_H
#define _COMMON_H

#ifdef x86
#include <jsoncpp/json/json.h>
#else

#include <json/json.h>

#endif

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
#define STR(p) p?p:""

    /**
    * 打印hex输出
    * @param data
    * @param len
    */
    void PrintHex(uint8_t *data, uint32_t len);

    /**
     * 产生uuid
     * @return uuid的string
     */
    string random_uuid(string seed = "89ab");


    /**
     * 整体通信大帧结构
     *  1byte   1byte   1byte       2bytes  4bytes  4bytes  Nbytes      2bytes
     *  帧头      版本  命令标识符       帧号  设备编号    帧长  json格式正文    校验码
     */


    enum CmdType {
        CmdResponse = 0x00,//应答指令
        CmdLogin = 0x01,//设备登录
        CmdHeartBeat = 0x02,//心跳
        CmdFusionData = 0x03,//监控实时数据
        CmdCrossTrafficJamAlarm = 0x04,//交叉路口堵塞报警
        CmdLineupInfoGather = 0x05,//排队长度等信息
        CmdPicData = 0x06,//设备视频数据回传
        CmdTrafficFlowGather = 0x07,//车流量统计
        CmdCarTrackGather = 0x08,//车辆轨迹
        CmdUnknown = 0xff,
    };//命令字类型

    //基本解包组包函数
    enum Direction {
        Unknown = 0,//未知
        East = 1,//东
        South = 2,//南
        West = 3,//西
        North = 4,//北
    };
#pragma pack(1)
    typedef struct {
        uint8_t tag = '$';//固定的头开始 ‘$’ 0x24
        uint8_t version;//版本号 1.0 hex
        uint8_t cmd = CmdUnknown;//命令字类型 详见CmdType
        uint16_t sn = 0;//帧号
        uint32_t deviceNO;//设备编号
        uint32_t len = 0;//整包长度，从包头到最后的校验位 <帧头>sizeof(PkgHead)+<正文>(json)+<校验>sizeof(PkgCRC)
    } PkgHead;//包头
    typedef struct {
        uint16_t data;//校验值，从帧头开始到正文的最后一个字节
    } PkgCRC;
#pragma pack()

    //一帧数据格式 <帧头>PkgHead+<正文>string(json)+<校验>PkgCRC
    typedef struct {
        PkgHead head;
        string body;
        PkgCRC crc;
    } Pkg;

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

    typedef struct {
        int state;// `json "state"`
        string desc;// `json "desc"`
        string value;// `json "value"`
    } Reply;//回复帧

    typedef struct {
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timestamp;// `json "timstamp"` 自1970.1.1 00:00:00到当前的秒数 date +%s获取秒数 date -d @秒数获取时间格式
    } Beats;//心跳帧 "Beats"

    //实时数据
    class AnnuciatorInfo {
    public:
        int LightID;//`json "LightID"`
        string Light;//`json "Light"`
        int RT;//`json "RT"`
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//信号机属性

    class ObjTarget {
    public:
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
        double speedX;
        double speedY;
        double longitude;//经度
        double latitude;//维度
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//目标属性

    class WatchData {
    public:
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
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//监控数据,对应命令字DeviceData

    class OneRvWayObject {
    public:
        int wayNo;//对应 Direction
        int roID;//雷达目标编号
        int voID;//视频目标编号
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class VideoTargets {
    public:
        int cameraObjID;
        int left = 0;//坐标 左
        int top = 0;//坐标 上
        int right = 0;// 坐标 右
        int bottom = 0;//坐标 下
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class VideoData {
    public:
        string rvHardCode;
        vector<VideoTargets> lstVideoTargets;
        string imageData;//图像数据
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class ObjMix {
    public:
        int objID = 0;//目标ID
        vector<OneRvWayObject> rvWayObject;
        int objType = 0;//目标类型
        int objColor = 0;//目标颜色
        string plates;//车牌号
        string plateColor;//车牌颜色
        float distance;//距离
        float angle;//航角度
        float speed;//速度
        double locationX = 0.0;//平面X位置
        double locationY = 0.0;//平面Y位置
        double longitude = 0.0;//经度
        double latitude = 0.0;//维度
        int flagNew = 0;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//融合后的目标数据

    class FusionData {
    public:
        string oprNum;// `json "oprNum"`
        double timstamp;// `json "timstamp"`自1970.1.1 00:00:00到当前的毫秒数
        string crossID;// ``json "crossID"路口编号
        int isHasImage;//`json "isHasImage"` 是否包含图像
        vector<ObjMix> lstObjTarget;// `json "lstObjTarget"`目标分类
        vector<VideoData> lstVideos;//图像数据，包括图像对应的设备编号、图像识别列表、图像base编码
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//多路融合数据,对应命令字DeviceData

    /**
     * 组包融合后的数据
     * @param fusionData 融合后数据结构体
     * @param sn 帧号
     * @param deviceNO 设备编号
     * @param pkg out 包结构体
     * @return 0：success -1：fail
     */
    int PkgFusionDataWithoutCRC(FusionData fusionData, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    //车流量统计
    class OneFlowData {
    public:
        string laneCode;
        int laneDirection;//车道方向
        int flowDirection;
        int inCars;
        double inAverageSpeed;//入口平均速度
        int outCars;
        double outAverageSpeed;//出口平均速度
        int queueLen;
        int queueCars;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class TrafficFlow {
    public:
        string oprNum;// `json "oprNum"`
        string crossID;
        string hardCode;
        double timestamp;// `json "timstamp"`自1970.1.1 00:00:00到当前的毫秒数
        vector<OneFlowData> flowData;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class TrafficFlowGather {
    public:
        string oprNum;// `json "oprNum"`
        string crossID;
        double timestamp;// `json "timestamp"`自1970.1.1 00:00:00到当前的毫秒数
        string recordDateTime;
        vector<OneFlowData> trafficFlow;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    int PkgTrafficFlowGatherWithoutCRC(TrafficFlowGather trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    //车辆轨迹
    class OneCarTrack {
    public:
        int id;
        int type;
        int cameraDirection;
        int x1;
        int y1;
        int x2;
        int y2;
        int latitude;
        int longitude;
        string laneCode;
        int speed;
        int timeHeadway;
        string plateNumber;
        string plateColor;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class CarTrack {
    public:
        string oprNum;// `json "oprNum"` uuid()
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timestamp;//`json "timstamp"` 自1970.1.1 00:00:00到当前的毫秒数
        string crossID;
        string ip;
        int type;
        vector<OneCarTrack> lstObj;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//车辆轨迹

    class CarTrackGather {
    public:
        string oprNum;// `json "oprNum"`
        string crossID;
        double timestamp;// `json "timestamp"`自1970.1.1 00:00:00到当前的毫秒数
        string recordDateTime;
        vector<OneCarTrack> lstObj;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    int PkgCarTrackGatherWithoutCRC(CarTrackGather carTrackGather, uint16_t sn, uint32_t deviceNO, Pkg &pkg);


    //---------交叉路口堵塞报警---------//
    class CrossTrafficJamAlarm {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        int alarmType;//1：交叉口堵塞
        int alarmStatus;//1 有报警 0 报警恢复
        string alarmTime;//日期2022-10-31 10:00:21
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    int PkgCrossTrafficJamAlarmWithoutCRC(CrossTrafficJamAlarm crossTrafficJamAlarm, uint16_t sn, uint32_t deviceNO,
                                          Pkg &pkg);

    //-------排队长度等信息--------//
    class OneLineupInfo {
    public:
        int laneID;
        int averageSpeed;
        int flow;
        int queueLength;
        int sumMini;
        int sumMid;
        int sumMax;
        int headWay;
        int headWayTime;
        int occupancy;
        int occupancySpace;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class LineupInfo {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        string recordDateTime;
        vector<OneLineupInfo> trafficFlowList;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class LineupInfoGather {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        string recordDateTime;
        vector<OneLineupInfo> trafficFlowList;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    int PkgLineupInfoGatherWithoutCRC(LineupInfoGather lineupInfoGather, uint16_t sn, uint32_t deviceNO, Pkg &pkg);
}

#endif //_COMMON_H
