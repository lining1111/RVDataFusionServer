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

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define OFFSET(type, member)      \
    ( (size_t)( &( ((type*)0)->member)  ) )
#define MEMBER_SIZE(type, member) \
    sizeof(((type *)0)->member)

    enum PkgType {
        Request = 0x00,//请求
        Response = 0x01,//回复
    };//包类型


#pragma pack(1)
    typedef struct {
        uint8_t tag = '$';//固定的头开始 ‘$’ 0x24
        uint8_t version;//版本号 1.0 hex
        uint8_t type;//包类型 详见PkgType
        uint16_t sn = 0;//包号
        uint32_t len = 0;//整包长度，从包头到最后的校验位 <帧头>sizeof(PkgHead)+<正文>(1+方法名长度+4+方法参数)+<校验>sizeof(PkgCRC)
    } PkgHead;//包头

    typedef struct {
        uint16_t data;//校验值，从帧头开始到正文的最后一个字节
    } PkgCRC;
#pragma pack()

    //方法名枚举
#define Method(s) #s

    typedef struct {
        uint8_t len = 0;
        string name;//Method(Beats) Method(DeviceStates) Method(DeviceAlarm) Method(CommanderAlarm) Method(BusinessData) Method(WatchData) Method(SendVideoInfo)
    } MethodName;//方法名

    typedef struct {
        uint32_t len = 0;
        string param;
    } MethodParam;//方法参数

    typedef struct {
        MethodName methodName;
        MethodParam methodParam;
    } PkgBody;


    //一帧数据格式 <帧头>PkgHead+<正文>PkgBody(1+方法名长度+4+方法参数)+<校验>PkgCRC
    typedef struct {
        PkgHead head;
        PkgBody body;
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
        double timestamp;// `json "timestamp"` 自1970.1.1 00:00:00到当前的秒数 date +%s获取秒数 date -d @秒数获取时间格式
    } Beats;//心跳帧 "Beats"

    typedef struct {
        int LightID;//`json "LightID"`
        string Light;//`json "Light"`
        int RT;//`json "RT"`
    } AnnuciatorInfo;//信号机属性

    typedef struct {
        int objID;//目标ID
        int objType;//目标类型
        string plates;//车牌号
        string plateColor;//车牌颜色
        int left;//坐标 左
        int top;//坐标 上
        int right;// 坐标 右
        int bottom;//坐标 下
        double locationX;
        double locationY;
        string distance;//距离
        string directionAngle;//航角度
        string speed;//速度
    } ObjTarget;//目标属性

    typedef struct {
        string oprNum;// `json "oprNum"` uuid()
        string hardCode;// `json "hardCode"` 设备唯一标识
        double timstamp;//`json "timstamp"` 自1970.1.1 00:00:00到当前的秒数
        string matrixNo;// `json "matrixNo"` 矩阵编号
        string cameraIp;// `json "cameraIp"` 相机编号
        double RecordDateTime;//`json "RecordDateTime"` 抓拍时间
        int isHasImage;//`json "isHasImage"` 是否包含图像
        string imageData;//`json "imageData"` 当前的视频图像数据
        vector<AnnuciatorInfo> listAnnuciatorInfo;//`json "AnnuciatorInfo"` 信号机列表
        vector<ObjTarget> lstObjTarget;//`json "lstObjTarget"` 目标分类
    } WatchData;//监控数据


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
     * 组包函数
     * @param pkg 帧数据
     * @param out 组帧后的数据地址
     * @param len 组帧后的数据长度
     * @return 0：success -1：fail
     */
    int Pack(Pkg pkg, uint8_t *out, uint32_t *len);

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

}


#endif //_COMMON_H
