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
    int objID;
    int cameraID;
    int objType;
    char plate_number[15];
    char plate_color[7];
    int left;
    int top;
    int right;
    int bottom;
    double locationX;
    double locationY;
    char distance[10];
    double directionAngle;
//    double speed;
    double speedX;
    double speedY;
    double longitude;//经度
    double latitude;//纬度
    //斜路口再用
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
    int objID1;
    int objID2;
    int objID3;
    int objID4;
    int showID;
    int cameraID1;
    int cameraID2;
    int cameraID3;
    int cameraID4;
    int objType;
    char plate_number[15];
    char plate_color[7];
    int left;
    int top;
    int right;
    int bottom;
    double locationX;
    double locationY;
    char distance[10];
    double directionAngle;
    double speed;
    double longitude;//经度
    double latitude;//纬度
    int flag_new;//判断当前目标id是否第一次出现
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


int merge_total(double repateX, double widthX, double widthY, double Xmax, double Ymax, double gatetx, double gatety,
                double gatex, double gatey, bool time_flag, OBJECT_INFO_T *Data_one, int n1, OBJECT_INFO_T *Data_two,
                int n2, OBJECT_INFO_T *Data_three, int n3, OBJECT_INFO_T *Data_four, int n4,
                OBJECT_INFO_NEW *Data_before1, int n_before1, OBJECT_INFO_NEW *Data_before2, int n_before2,
                OBJECT_INFO_NEW *Data_out, double angle_value);

/*
//斜路口
int merge_total(int flag_view, double left_down_x, double left_down_y, double left_up_x, double left_up_y,
                double right_up_x, double right_up_y, double right_down_x, double right_down_y, double repateX,
                double repateY, double gatetx, double gatety, double gatex, double gatey, bool time_flag,
                OBJECT_INFO_T *Data_one, int n1, OBJECT_INFO_T *Data_two, int n2, OBJECT_INFO_T *Data_three, int n3,
                OBJECT_INFO_T *Data_four, int n4, OBJECT_INFO_NEW *Data_before1, int n_before1,
                OBJECT_INFO_NEW *Data_before2, int n_before2, OBJECT_INFO_NEW *Data_out, double angle_value);
*/

#endif //_MERGE_H
