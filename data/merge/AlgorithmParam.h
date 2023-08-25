//
// Created by lining on 6/30/22.
//

#ifndef _ALGORITHMPARAM_H
#define _ALGORITHMPARAM_H

#include <json/json.h>

#include <vector>
#include <string>
using namespace std;

//算法配置相关
class PointF {
public:
    double x;
    double y;

    PointF() {

    }

    PointF(double x, double y) : x(x), y(y) {

    }

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class PointFArray {
public:
    int index = 0;//属于第几组
    vector<PointF> points;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};


class DoubleValue {
public:
    int index = 0;
    double value;

    DoubleValue() {

    }

    DoubleValue(int index, double value) : index(index), value(value) {

    }

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class CorrectedValueGPS {
public:
    int index = 0;
    double latitude = 0.0;
    double longitude = 0.0;

    CorrectedValueGPS() {

    }

    CorrectedValueGPS(int index, double latitude, double longitude)
            : index(index), latitude(latitude), longitude(longitude) {

    }

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class RectFValue {
public:
    int index = 0;
    double x;
    double y;
    double width;
    double height;

    RectFValue() {

    }

    RectFValue(int index, double x, double y, double width, double height)
            : index(index), x(x), y(y), width(width), height(height) {

    }

public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};


class RectF {
public:
    double x;
    double y;
    double width;
    double height;

    RectF() {

    }

    RectF(double x, double y, double width, double height)
            : x(x), y(y), width(width), height(height) {

    }

public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};


class AlgorithmParam {
public:
    //各个路口的矩阵
    vector<DoubleValue> H_east;//西向东侧路口
    vector<DoubleValue> H_north;//南向北侧路口
    vector<DoubleValue> H_west;//东向西侧路口
    vector<DoubleValue> H_south;//北向南侧路口
    //将数组设置到类内变量
    void setH_XX(vector<DoubleValue> &H_xx, vector<double> values);

    //将类内变量设置到数组
    void getH_XX(vector<DoubleValue> H_xx, vector<double> &values);

    //每个路口的中心经纬度
    int start_utm_x = 325235;
    int start_utm_y = 4512985;
    double crossroad_mid_longitude;//经度
    double crossroad_mid_latitude;//纬度
    //路口中心距离每侧停止线的距离（m）
    int distance_of_east_stopline_from_roadcenter = 20;
    int distance_of_west_stopline_from_roadcenter = 20;
    int distance_of_north_stopline_from_roadcenter = 20;
    int distance_of_south_stopline_from_roadcenter = 20;
    int east_west_selection_criteria = 0;
    //每侧路口的倾斜角度
    vector<DoubleValue> angle;

    void setAngle(vector<double> values);

    void getAngle(vector<double> &values);

    //有倾斜路口时需要划定区域
    vector<PointFArray> east_west_driving_area;   //东向西行驶的车辆
    vector<PointFArray> west_east_driving_area;   //西向东行驶的车辆
    vector<PointFArray> north_south_driving_area; //北向南行驶的车辆
    vector<PointFArray> south_north_driving_area; //南向北行驶的车辆

    void setXX_driving_are(vector<PointFArray> &xx, vector<vector<PointF>> values);

    void getXX_driving_are(vector<PointFArray> xx, vector<vector<PointF>> &values);

    int piexl_type = 0;
    int min_track_distance = 200;
    int max_track_distance = 400;
    double area_ratio = 0.8;
    int failCount1 = 10;
    int failCount2 = 2;
    double piexlbymeter_x = 10.245;
    double piexlbymeter_y = 7.76;
    double speed_moving_average = 0.1;
    int min_intersection_area = 50;
    int road_length = 100;
    double shaking_pixel_threshold = 0.1;
    int stopline_length = 5;
    int track_in_length = 50;
    int track_out_length = 20;
    double fusion_min_distance = 130;
    double fusion_max_distance = 180;
    int max_speed_by_piexl = 20;
    int car_match_count = 2;
    double max_center_distance_threshold = 0.5;
    double min_center_distance_threshold = 0.3;
    double min_area_threshold = 0.3;
    double max_area_threshold = 0.7;
    double middle_area_threshold = 0.6;
    double max_stop_speed_threshold = 0.6;
    double min_stop_speed_threshold = 0.4;


    vector<CorrectedValueGPS> correctedValueGPSs;


    vector<RectFValue> north_west_driving_missing_area;
    vector<RectFValue> east_north_driving_missing_area;

    vector<RectFValue> west_south_driving_missing_area;
    vector<RectFValue> south_east_driving_missing_area;

    vector<RectFValue> north_west_driving_in_area;
    vector<RectFValue> east_north_driving_in_area;

    vector<RectFValue> west_south_driving_in_area;
    vector<RectFValue> south_east_driving_in_area;

    //将数组设置到类内变量
    void set_area(vector<RectFValue> &xx, vector<RectF> values);

    //将类内变量设置到数组
    void get_area(vector<RectFValue> xx, vector<RectF> &values);


    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

//    //转化为ENW.h的结构体
//    void getFromENW_ImageUnifiedParamsENW(class ImageUnifiedParamsENW in);
//
//    void setToENW_ImageUnifiedParamsENW(class ImageUnifiedParamsENW &out);
};


#endif //_MERGESTRUCT_H
