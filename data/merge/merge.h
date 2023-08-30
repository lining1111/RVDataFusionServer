//
// Created by lining on 2/18/22.
//

#ifndef _MERGE_H
#define _MERGE_H

#include "common/common.h"

#include <string>
#include <stdint.h>
#include <string.h>

using namespace common;
#define PI 3.1415926
typedef struct {
    int objID = 0;
    std::string inputID;
    int cameraID = 0;
    int objSourceType = 0;//0:相机 1：雷达 2：多目
    int objType;//目标类型
    //添加属性
    string laneCode;
    float carLength;//车长,只会在停止线附近给一次估算值,其他时刻都是0
    string carFeaturePic;//车辆特写图（Base64编码）,只会在停止线附近清楚的位置从1920*1080分辨的原图上抠一张车辆特写图,不会重复发送。不发送的时刻都是空
    string plates;//车牌号
    string plateColor;//车牌颜色
    int carType = 0;//车辆类型

    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    double locationX = 0.0;
    double locationY = 0.0;
    // char distance[10];
    double directionAngle = 0.0;
//    double speed;
    double speedX = 0.0;
    double speedY = 0.0;
    double longitude = 0.0;//经度
    double latitude = 0.0;//纬度
    //

    int radarID = 0;
    char plate_number[15];
    char plate_color[7];
    char distance[10];

//    double dmerge_left_up_x;
//    double dmerge_left_up_y;
//    double dmerge_right_up_x;
//    double dmerge_right_up_y;
//    double dmerge_left_down_x;
//    double dmerge_left_down_y;
//    double dmerge_right_down_x;
//    double dmerge_right_down_y;
} OBJECT_INFO_T;

typedef struct {
    int objID1 = 0;
    int objID2 = 0;
    int objID3 = 0;
    int objID4 = 0;
    int cameraID1 = 0;
    int cameraID2 = 0;
    int cameraID3 = 0;
    int cameraID4 = 0;
    int objSourceType = 0;//0:相机 1：雷达 2：多目
    uint64_t time_stamp = 0;//新增时间戳
    int objType = 0;//目标类型
    //添加属性
    string laneCode;
    float carLength = 0.0;//车长,只会在停止线附近给一次估算值,其他时刻都是0
    string carFeaturePic;//车辆特写图（Base64编码）,只会在停止线附近清楚的位置从1920*1080分辨的原图上抠一张车辆特写图,不会重复发送。不发送的时刻都是空
    string plates;//车牌号
    string plateColor;//车牌颜色
    int carType = 0;//车辆类型

    int showID = 0;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    double directionAngle = 0.0;
    double speed = 0.0;
    double longitude = 0.0;//经度
    double latitude = 0.0;//纬度

    double locationX = -1;
    double locationY = -1;
    int flag_new = -1;//判断当前目标id是否第一次出现

    vector<int> objID;
    vector<int> radarID;
    vector<int> cameraID;
    char plate_number[15];
    char plate_color[7];
    char distance[10];

    double speedX = 0.0;
    double speedY = 0.0;
    int continue_num = 0;
    int fail_num = 0;
    int flag_track = 0;
    int existFrame = 0;//同一ID出现的帧数
    //斜路口再用
//    double dmerge_left_up_x;
//    double dmerge_left_up_y;
//    double dmerge_right_up_x;
//    double dmerge_right_up_y;
//    double dmerge_left_down_x;
//    double dmerge_left_down_y;
//    double dmerge_right_down_x;
//    double dmerge_right_down_y;
} OBJECT_INFO_NEW;


#endif //_MERGE_H
