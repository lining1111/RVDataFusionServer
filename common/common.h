//
// Created by lining on 2/16/22.
//

#ifndef _COMMON_H
#define _COMMON_H

#include <json/json.h>
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


#define ComVersion "1.1.0"

    string GetComVersion();


    enum CmdType {
        CmdResponse = 0x00,//应答指令
        CmdControl = 0x01,//设备控制指令--视频数据回传
        CmdHeartBeat = 0x02,//心跳
        CmdFusionData = 0x03,//监控实时数据
        CmdCrossTrafficJamAlarm = 0x04,//交叉路口堵塞报警
        CmdAbnormalStopData = 0x05,//异常停车数据
        CmdIntersectionOverflowAlarm = 0x06,//路口溢出报警
        CmdTrafficFlowGather = 0x07,//车流量统计
        CmdInWatchData_1_3_4 = 0x08,//进口监控数据（1）（3）（4）触发式上报
        CmdInWatchData_2 = 0x09,//进口监控数据（2）周期上报
        CmdStopLinePassData = 0x0a,//停止线过车数据
        CmdLongDistanceOnSolidLineAlarm = 0x0b,//长距离压实线报警
        CmdHumanData = 0x0c,//3516相机预警信息 行人感知
        CmdHumanLitPoleData = 0x0d,//人形灯杆数据
        CmdUnknown = 0xff,
    };//命令字类型

    //基本解包组包函数

//    enum Direction {
//        Unknown = 0,//未知
//        East = 1,//东
//        South = 2,//南
//        West = 3,//西
//        North = 4,//北
//    };
    //方向枚举1=东,2=西,3=南,4=北,5=东北,6=西南,7=东南,8=西北
    enum Direction {
        Unknown = 0,//未知
        East = 1,//东
        West = 2,//西
        South = 3,//南
        North = 4,//北
        Northeast = 5,//东北
        Southwest = 6,//西南
        Southeast = 7,//东南
        Northwest = 8,//西北
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

    class PkgClass {
    public:
        CmdType cmdType;

        virtual bool JsonMarshal(Json::Value &out) = 0;

        virtual bool JsonUnmarshal(Json::Value in) = 0;

        int PkgWithoutCRC(uint16_t sn, uint32_t deviceNO, Pkg &pkg);
    };

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

    class Control : public PkgClass {
    public:
        string oprNum;// `json "oprNum"` uuid()
        double timestamp;//`json "timstamp"` 自1970.1.1 00:00:00到当前的毫秒数
        int isSendVideoInfo;//0 不回传 1 回传
        int videoType;
    public:
        Control() {
            this->cmdType = CmdControl;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };


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
        string objID;//目标ID
        int objCameraID = 0;//摄像头目标ID
        int objRadarID = 0;//雷达目标ID
        int objSourceType = 0;//0:相机 1：雷达 2：多目
        int objType = 0;//目标类型
        string plates;//车牌号
        string plateColor;//车牌颜色

        int carType = 0;
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
        string laneCode;
        double carLength;//车长,只会在停止线附近给一次估算值,其他时刻都是0
        string carFeaturePic;//车辆特写图（Base64编码）,只会在停止线附近清楚的位置从1920*1080分辨的原图上抠一张车辆特写图,不会重复发送。不发送的时刻都是空
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//目标属性

    class WatchData : public PkgClass {
    public:
        string oprNum;// `json "oprNum"` uuid()
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timstamp = 0;//`json "timstamp"` 自1970.1.1 00:00:00到当前的毫秒数
        string matrixNo;// `json "matrixNo"` 矩阵编号
        string cameraIp;// `json "cameraIp"` 相机编号
        double RecordDateTime;//`json "RecordDateTime"` 抓拍时间
        int isHasImage;//`json "isHasImage"` 是否包含图像
        string imageData;//`json "imageData"` 当前的视频图像数据
        int direction;//方向，枚举类型 enum Direction
        int roadDirection;
        vector<AnnuciatorInfo> listAnnuciatorInfo;//`json "AnnuciatorInfo"` 信号机列表
        vector<ObjTarget> lstObjTarget;//`json "lstObjTarget"` 目标分类
    public:
        WatchData() {
            this->cmdType = CmdFusionData;
        }

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
        int direction;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class ObjMix {
    public:
        string objID;//目标ID
        vector<OneRvWayObject> rvWayObject;
        int objType = 0;//目标类型 0.汽车1.人2.摩托3.自行车4.三轮
        string objColor;//目标颜色
        int carType = 0;//车辆类型 0.非汽车1.大型车2.中型车3.小型车4.货车5.公交车算法暂时识别不出来默认填0
        string plates;//车牌号
        string plateColor;//车牌颜色
        float distance = 0.0;//距离
        float angle = 0.0;//航角度
        float speed = 0.0;//速度
        double locationX = 0.0;//平面X位置
        double locationY = 0.0;//平面Y位置
        double longitude = 0.0;//经度
        double latitude = 0.0;//纬度
        string laneCode;//车道号
        double carLength = 0.0;//车长,只会在停止线附近给一次估算值,其他时刻都是0
        string carFeaturePic;//车辆特写图（Base64编码）,只会在停止线附近清楚的位置从1920*1080分辨的原图上抠一张车辆特写图,不会重复发送。不发送的时刻都是空
        int flagNew = 0;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };//融合后的目标数据

    class FusionData : public PkgClass {
    public:
        string oprNum;// `json "oprNum"`
        double timstamp;// `json "timstamp"`自1970.1.1 00:00:00到当前的毫秒数
        string crossID;// ``json "crossID"路口编号
        int isHasImage;//`json "isHasImage"` 是否包含图像
        vector<ObjMix> lstObjTarget;// `json "lstObjTarget"`目标分类
        vector<VideoData> lstVideos;//图像数据，包括图像对应的设备编号、图像识别列表、图像base编码
    public:
        FusionData() {
            this->cmdType = CmdFusionData;
        }

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
//    int PkgFusionDataWithoutCRC(FusionData fusionData, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    //车流量统计
    class OneFlowData {
    public:
        string laneCode;//车道编号
        int laneDirection;//车道方向
        int flowDirection;
        int inCars;//进入数量
        double inAverageSpeed;//入口平均速度
        int outCars;//驶出数量
        double outAverageSpeed;//出口平均速度
        int queueLen;//排队长度
        int queueCars;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class TrafficFlow : public PkgClass {
    public:
        string oprNum;// `json "oprNum"`
        string crossID;
        string hardCode;
        double timestamp;// `json "timestamp"`自1970.1.1 00:00:00到当前的毫秒数
        vector<OneFlowData> flowData;
    public:
        TrafficFlow() {
            this->cmdType = CmdTrafficFlowGather;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class TrafficFlowGather : public PkgClass {
    public:
        string oprNum;// `json "oprNum"`
        string crossID;
        double timestamp;// `json "timestamp"`自1970.1.1 00:00:00到当前的毫秒数
        string recordDateTime;
        vector<OneFlowData> trafficFlow;
    public:
        TrafficFlowGather() {
            this->cmdType = CmdTrafficFlowGather;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

//    int PkgTrafficFlowGatherWithoutCRC(TrafficFlowGather trafficFlows, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    //---------交叉路口堵塞报警---------//
    class CrossTrafficJamAlarm : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
//        int roadDirection;//0 东向西1 西向东2 南向北3 北向南
        int alarmType;//1：交叉口堵塞
        int alarmStatus;//1 有报警 0 报警恢复
        string alarmTime;//日期2022-10-31 10:00:21
    public:
        CrossTrafficJamAlarm() {
            this->cmdType = CmdCrossTrafficJamAlarm;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

//    int PkgCrossTrafficJamAlarmWithoutCRC(CrossTrafficJamAlarm crossTrafficJamAlarm, uint16_t sn, uint32_t deviceNO,
//    Pkg &pkg
//    );

    //--路口溢出报警上传--//
    class IntersectionOverflowAlarm : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        int roadDirection;//0 东向西1 西向东2 南向北3 北向南
        int alarmType;//1：交叉口堵塞
        int alarmStatus;//1 有报警 0 报警恢复
        string alarmTime;//日期2022-10-31 10:00:21
    public:
        IntersectionOverflowAlarm() {
            this->cmdType = CmdIntersectionOverflowAlarm;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

//    int PkgIntersectionOverflowAlarmWithoutCRC(IntersectionOverflowAlarm intersectionOverflowAlarm, uint16_t sn,
//                                               uint32_t deviceNO, Pkg &pkg);

    //透传类型
    class InWatchData_1_3_4 : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        string laneCode;//车道编号
        int laneDirection;//车道方向 1=东，2=南，3=西，4=北 同EOC一致
        int flowDirection;//流向方向 1=直行车道，2=左转车道，3=右转车道，4=掉头车道，5=直行-左转车道，6=直行右转车道，7=直行-掉头车道，8=左转-掉头车道，9=直行-左转-掉头车道 同EOC一致
        int detectLocation;//触发线位置 0停止线 42检测线 60检测线
        int vehicleID;//车辆ID
        int vehicleType;//车辆类型
        int vehicleSpeed;//车辆速度
    public:
        InWatchData_1_3_4() {
            this->cmdType = CmdInWatchData_1_3_4;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

//    int PkgInWatchData_1_3_4WithoutCRC(InWatchData_1_3_4 inWatchData134, uint16_t sn, uint32_t deviceNO, Pkg &pkg);

    //透传类型
    class InWatchData_2_trafficFlowListItem_vehicleIDListItem {
    public:
        int vehicleID;//车辆ID
        int vehicleType;//车辆类型 0.非汽车1.小型汽车2.大型（公交）3.大型（渣土）4.大型（其他）5.其他 算法暂时识别不出来默认填0
        int vehicleSpeed;//车辆速度
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class InWatchData_2_trafficFlowListItem {
    public:
        string laneCode;//车道编号
        int laneDirection;//车道方向 1=东，2=南，3=西，4=北 同EOC一致
        int flowDirection;//流向方向 1=直行车道，2=左转车道，3=右转车道，4=掉头车道，5=直行-左转车道，6=直行右转车道，7=直行-掉头车道，8=左转-掉头车道，9=直行-左转-掉头车道 同EOC一致
        vector<InWatchData_2_trafficFlowListItem_vehicleIDListItem> vehicleIDList;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class InWatchData_2 : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        int recordLaneSum;//记录上报车道数量
        vector<InWatchData_2_trafficFlowListItem> trafficFlowList;//
    public:
        InWatchData_2() {
            this->cmdType = CmdInWatchData_2;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    //停止线过车数据
    class StopLinePassData_vehicleListItem {
    public:
        string laneCode;//车道编号
        int laneDirection;//车道方向 1=东，2=南，3=西，4=北 同EOC一致
        int flowDirection;//流向方向
        string vehiclePlate;//车牌号
        string vehiclePlateColor;//车牌颜色
        int vehicleID;//车辆ID
        int vehicleType;//车辆类型
        int vehicleSpeed;//车辆速度

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };


    class StopLinePassData : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        vector<StopLinePassData_vehicleListItem> vehicleList;
    public:
        StopLinePassData() {
            this->cmdType = CmdStopLinePassData;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    //异常停车报警
    class AbnormalStopData : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        string laneCode;
        int alarmType;
        int alarmStatus;
        string alarmTime;
    public:
        AbnormalStopData() {
            this->cmdType = CmdAbnormalStopData;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    //长距离压实线报警上传
    class LongDistanceOnSolidLineAlarm : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        string laneCode;
        double longitude;
        double latitude;
        int alarmType;
        int alarmStatus;
        string alarmTime;
    public:
        LongDistanceOnSolidLineAlarm() {
            this->cmdType = CmdLongDistanceOnSolidLineAlarm;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    //行人数据
    class HumanData_areaListItem_pointListItem {
        int x;
        int y;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class HumanData_areaListItem {
    public:
        vector<HumanData_areaListItem_pointListItem> pointList;
        int humanNum;
        int humanType;
        int bicycleNum;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };


    class HumanData : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        int location;
        int direction;
        vector<HumanData_areaListItem> areaList;
    public:
        HumanData() {
            this->cmdType = CmdHumanData;
        };

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class HumanDataGather_deviceListItem {
    public:
        string deviceCode;
        int detectDirection;
        int direction;
        int humanNum;
        int humanType;
        int bicycleNum;
    public:
        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class HumanDataGather : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        vector<HumanDataGather_deviceListItem> deviceList;
    public:
        HumanDataGather() {
            this->cmdType = CmdHumanData;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    //人形灯杆数据，透传
    class HumanLitPoleData_deviceListItem {
    public:
        string deviceCode;//设备编号
        int detectDirection;//检查方位
        int direction;//流向
        int humanNum;//等待区人数
        int humanFlow;//穿行人数

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

    class HumanLitPoleData : public PkgClass {
    public:
        string oprNum;
        double timestamp;
        string crossID;
        string hardCode;
        vector<HumanLitPoleData_deviceListItem> deviceList;
    public:
        HumanLitPoleData() {
            this->cmdType = CmdHumanLitPoleData;
        }

        bool JsonMarshal(Json::Value &out);

        bool JsonUnmarshal(Json::Value in);
    };

}

#endif //_COMMON_H
