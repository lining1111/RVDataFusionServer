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
    char directionAngle[10];
    double speed;
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


void OBJECT_INFO_T2ObjTarget(OBJECT_INFO_T &objectInfoT, ObjTarget &objTarget);

void ObjTarget2OBJECT_INFO_T(ObjTarget &objTarget, OBJECT_INFO_T &objectInfoT);

void OBJECT_INFO_T2OBJECT_INFO_NEW(OBJECT_INFO_T &objectInfoT, OBJECT_INFO_NEW &objectInfoNew);

int merge_total(double repateX, double widthX, double widthY, double Xmax, double Ymax, double gatetx, double gatety,
                double gatex, double gatey, bool time_flag, OBJECT_INFO_T *Data_one, int n1, OBJECT_INFO_T *Data_two,
                int n2, OBJECT_INFO_T *Data_three, int n3, OBJECT_INFO_T *Data_four, int n4,
                OBJECT_INFO_NEW *Data_before1, int n_before1, OBJECT_INFO_NEW *Data_before2, int n_before2,
                OBJECT_INFO_NEW *Data_out, double angle_value);


#endif //_MERGE_H
