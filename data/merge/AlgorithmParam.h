//
// Created by lining on 6/30/22.
//

#ifndef _ALGORITHMPARAM_H
#define _ALGORITHMPARAM_H

#include <vector>
#include <string>
#include <algorithm>

using namespace std;

#include <xpack/json.h>

using namespace xpack;

//算法配置相关
class PointF {
public:
    double x = 0.0;
    double y = 0.0;

    PointF() {

    }

    PointF(double x, double y) : x(x), y(y) {

    }

XPACK(O(x, y));
};

//带坐标的点信息
class PointFArray {
public:
    int index = 0;
    vector<PointF> value;

    PointFArray() {

    }

XPACK(O(index, value));

};

//带坐标的点集合信息
class PointFArrayArray {
public:
    int index = 0;
    vector<PointFArray> value;

    PointFArrayArray() {

    }

XPACK(O(index, value));

    void set(int index, vector<vector<PointF>> values) {
        this->index = index;
        for (int i = 0; i < values.size(); i++) {
            PointFArray pointFArray;
            pointFArray.index = i;
            auto iter = values.at(i);
            for (auto iter1: iter) {
                pointFArray.value.push_back(iter1);
            }
            this->value.push_back(pointFArray);
        }
    };

    void get(int &index, vector<vector<PointF>> &values) {
        this->index = index;
        if (this->value.empty()) {
            return;
        }
        //先将xx排序
        if (this->value.size() > 1) {
            std::sort(this->value.begin(), this->value.end(), [](PointFArray a, PointFArray b) {
                return a.index < b.index;
            });
        }

        for (auto iter: this->value) {
            vector<PointF> v_pointF;
            vector<PointF>().swap(v_pointF);
            for (auto iter1: iter.value) {
                v_pointF.push_back(iter1);
            }
            values.push_back(v_pointF);
        }
    };

};


//带坐标的double
class DoubleValue {
public:
    int index = 0;
    double value = 0.0;

    DoubleValue() {

    }

    DoubleValue(int index, double value) : index(index), value(value) {

    }

XPACK(O(index, value));

};

//带坐标的int
class IntValue {
public:
    int index = 0;
    int value = 0;

    IntValue() {

    }

    IntValue(int index, int value) : index(index), value(value) {

    }

XPACK(O(index, value));
};


//3X3转换矩阵
class DoubleValueArray {
public:
    int index = 0;
    vector<DoubleValue> value;

    DoubleValueArray() {

    }

XPACK(O(index, value));

    //将数组设置到类内变量
    void set(int index, vector<double> values) {
        this->index = index;
        for (int i = 0; i < values.size(); i++) {
            DoubleValue dv;
            dv.index = i;
            dv.value = values.at(i);
            this->value.push_back(dv);
        }
    };

    //将类内变量设置到数组
    void get(int &index, vector<double> &values) {
        index = this->index;
        if (this->value.empty()) {
            return;
        }
        //先将H_xx排序
        if (this->value.size() > 1) {
            std::sort(this->value.begin(), this->value.end(), [](DoubleValue a, DoubleValue b) {
                return a.index < b.index;
            });
        }
        for (auto iter: this->value) {
            values.push_back(iter.value);
        }
    };
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

XPACK(O(index, latitude, longitude));
};

class RectF {
public:
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;

    RectF() {

    }

    RectF(double x, double y, double width, double height)
            : x(x), y(y), width(width), height(height) {

    }

XPACK(O(x, y, width, height));

};


class RectFValue {
public:
    int index = 0;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;

    RectFValue() {

    }

    RectFValue(int index, double x, double y, double width, double height)
            : index(index), x(x), y(y), width(width), height(height) {

    }

XPACK(O(index, x, y, width, height));

};

class RectFValueArray {
public:
    int index = 0;
    vector<RectFValue> value;

    RectFValueArray() {

    }

XPACK(O(index, value));

//将数组设置到类内变量
    void set(int index, vector<RectF> values) {
        this->index = index;
        for (int i = 0; i < values.size(); i++) {
            RectFValue item;
            item.index = i;
            item.x = values.at(i).x;
            item.y = values.at(i).y;
            item.width = values.at(i).width;
            item.height = values.at(i).height;
            this->value.push_back(item);
        }
    };


    //将类内变量设置到数组
    void get(int &index, vector<RectF> &values) {
        index = this->index;

        if (this->value.empty()) {
            return;
        }
        //先将xx排序
        if (this->value.size() > 1) {
            std::sort(this->value.begin(), this->value.end(), [](RectFValue a, RectFValue b) {
                return a.index < b.index;
            });
        }

        for (auto iter: this->value) {
            RectF item;
            item.x = iter.x;
            item.y = iter.y;
            item.width = iter.width;
            item.height = iter.height;
            values.push_back(item);
        }
    };

};


//class AlgorithmParam {
//public:
//    //各个路口的矩阵
//    vector<DoubleValue> H_east;//西向东侧路口
//    vector<DoubleValue> H_north;//南向北侧路口
//    vector<DoubleValue> H_west;//东向西侧路口
//    vector<DoubleValue> H_south;//北向南侧路口
//    //将数组设置到类内变量
//    void setH_XX(vector<DoubleValue> &H_xx, vector<double> values){
//        for (int i = 0; i < values.size(); i++) {
//            DoubleValue dv;
//            dv.index = i;
//            dv.value = values.at(i);
//            H_xx.push_back(dv);
//        }
//    };
//
//
//    //将类内变量设置到数组
//    void getH_XX(vector<DoubleValue> H_xx, vector<double> &values){
//        if (H_xx.empty()) {
//            return;
//        }
//        //先将H_xx排序
//        if (H_xx.size() > 1) {
//            std::sort(H_xx.begin(), H_xx.end(), [](DoubleValue a, DoubleValue b) {
//                return a.index < b.index;
//            });
//        }
//        for (auto iter: H_xx) {
//            values.push_back(iter.value);
//        }
//    };
//
//
//    //每个路口的中心经纬度
//    int start_utm_x = 325235;
//    int start_utm_y = 4512985;
//    double crossroad_mid_longitude;//经度
//    double crossroad_mid_latitude;//纬度
//    //路口中心距离每侧停止线的距离（m）
//    int distance_of_east_stopline_from_roadcenter = 20;
//    int distance_of_west_stopline_from_roadcenter = 20;
//    int distance_of_north_stopline_from_roadcenter = 20;
//    int distance_of_south_stopline_from_roadcenter = 20;
//    int east_west_selection_criteria = 0;
//    //每侧路口的倾斜角度
//    vector<DoubleValue> angle;
//
//    void setAngle(vector<double> values){
//        for (int i = 0; i < values.size(); i++) {
//            DoubleValue dv;
//            dv.index = i;
//            dv.value = values.at(i);
//            this->angle.push_back(dv);
//        }
//    };
//
//    void getAngle(vector<double> &values){
//        if (angle.empty()) {
//            return;
//        }
//        //先将angle排序
//        if (angle.size() > 1) {
//            std::sort(angle.begin(), angle.end(), [](DoubleValue a, DoubleValue b) {
//                return a.index < b.index;
//            });
//        }
//
//        for (auto iter: angle) {
//            values.push_back(iter.value);
//        }
//    };
//
//    //有倾斜路口时需要划定区域
//    vector<PointFArray> east_west_driving_area;   //东向西行驶的车辆
//    vector<PointFArray> west_east_driving_area;   //西向东行驶的车辆
//    vector<PointFArray> north_south_driving_area; //北向南行驶的车辆
//    vector<PointFArray> south_north_driving_area; //南向北行驶的车辆
//
//    void setXX_driving_are(vector<PointFArray> &xx, vector<vector<PointF>> values){
//        for (int i = 0; i < values.size(); i++) {
//            PointFArray pointFArray;
//            pointFArray.index = i;
//            auto iter = values.at(i);
//            for (auto iter1: iter) {
//                pointFArray.points.push_back(iter1);
//            }
//            xx.push_back(pointFArray);
//        }
//    };
//
//
//    void getXX_driving_are(vector<PointFArray> xx, vector<vector<PointF>> &values){
//        if (xx.empty()) {
//            return;
//        }
//        //先将xx排序
//        if (xx.size() > 1) {
//            std::sort(xx.begin(), xx.end(), [](PointFArray a, PointFArray b) {
//                return a.index < b.index;
//            });
//        }
//
//        for (auto iter: xx) {
//            vector<PointF> v_pointF;
//            vector<PointF>().swap(v_pointF);
//            for (auto iter1: iter.points) {
//                v_pointF.push_back(iter1);
//            }
//            values.push_back(v_pointF);
//        }
//    };
//
//
//    int piexl_type = 0;
//    int min_track_distance = 200;
//    int max_track_distance = 400;
//    double area_ratio = 0.8;
//    int failCount1 = 10;
//    int failCount2 = 2;
//    double piexlbymeter_x = 10.245;
//    double piexlbymeter_y = 7.76;
//    double speed_moving_average = 0.1;
//    int min_intersection_area = 50;
//    int road_length = 100;
//    double shaking_pixel_threshold = 0.1;
//    int stopline_length = 5;
//    int track_in_length = 50;
//    int track_out_length = 20;
//    double fusion_min_distance = 130;
//    double fusion_max_distance = 180;
//    int max_speed_by_piexl = 20;
//    int car_match_count = 2;
//    double max_center_distance_threshold = 0.5;
//    double min_center_distance_threshold = 0.3;
//    double min_area_threshold = 0.3;
//    double max_area_threshold = 0.7;
//    double middle_area_threshold = 0.6;
//    double max_stop_speed_threshold = 0.6;
//    double min_stop_speed_threshold = 0.4;
//
//
//    vector<CorrectedValueGPS> correctedValueGPSs;
//
//
//    vector<RectFValue> north_west_driving_missing_area;
//    vector<RectFValue> east_north_driving_missing_area;
//
//    vector<RectFValue> west_south_driving_missing_area;
//    vector<RectFValue> south_east_driving_missing_area;
//
//    vector<RectFValue> north_west_driving_in_area;
//    vector<RectFValue> east_north_driving_in_area;
//
//    vector<RectFValue> west_south_driving_in_area;
//    vector<RectFValue> south_east_driving_in_area;
//
//    //将数组设置到类内变量
//    void set_area(vector<RectFValue> &xx, vector<RectF> values){
//        for (int i = 0; i < values.size(); i++) {
//            RectFValue item;
//            item.index = i;
//            item.x = values.at(i).x;
//            item.y = values.at(i).y;
//            item.width = values.at(i).width;
//            item.height = values.at(i).height;
//            xx.push_back(item);
//        }
//    };
//
//
//    //将类内变量设置到数组
//    void get_area(vector<RectFValue> xx, vector<RectF> &values){
//        if (xx.empty()) {
//            return;
//        }
//        //先将xx排序
//        if (xx.size() > 1) {
//            std::sort(xx.begin(), xx.end(), [](RectFValue a, RectFValue b) {
//                return a.index < b.index;
//            });
//        }
//
//        for (auto iter: xx) {
//            RectF item;
//            item.x = iter.x;
//            item.y = iter.y;
//            item.width = iter.width;
//            item.height = iter.height;
//            values.push_back(item);
//        }
//    };
//
//
//
//XPACK(O(H_east, H_north, H_west, H_south,
//        start_utm_x, start_utm_y, crossroad_mid_longitude, crossroad_mid_latitude,
//        distance_of_east_stopline_from_roadcenter,
//        distance_of_west_stopline_from_roadcenter,
//        distance_of_north_stopline_from_roadcenter,
//        distance_of_south_stopline_from_roadcenter,
//        east_west_selection_criteria,
//        angle,
//        east_west_driving_area, west_east_driving_area, north_south_driving_area, south_north_driving_area,
//        piexl_type, min_track_distance, max_track_distance, area_ratio, failCount1, failCount2,
//        piexlbymeter_x, piexlbymeter_y, speed_moving_average, min_intersection_area, road_length,
//        shaking_pixel_threshold, stopline_length, track_in_length, track_out_length,
//        fusion_min_distance, fusion_max_distance, max_speed_by_piexl, car_match_count,
//        max_center_distance_threshold, min_center_distance_threshold,
//        min_area_threshold, max_area_threshold, middle_area_threshold,
//        max_stop_speed_threshold, min_stop_speed_threshold,
//        correctedValueGPSs,
//        north_west_driving_missing_area, east_north_driving_missing_area,
//        west_south_driving_missing_area, south_east_driving_missing_area,
//        north_west_driving_in_area, east_north_driving_in_area,
//        west_south_driving_in_area, south_east_driving_in_area));
//};

class AlgorithmParam {
public:
    //0 路口名称
    string intersectionName;
    //1 路口转换矩阵
    vector<DoubleValueArray> transformMatrix;//转换矩阵

    //2 路口的倾斜角度
    vector<DoubleValue> angle;

    //3 路口的中心经纬度
    int startUTMX = 325235;
    int startUTMY = 4512985;
    double midLongitude;//经度
    double midLatitude;//纬度
    //4 路口中心距离每侧停止线的距离（m）
    vector<IntValue> distanceOfStoplineFromRoadcenter;
    //5 选中区域
    vector<IntValue> selectionCriteria;

    //6 驶入区域
    vector<PointFArrayArray> drivingArea;
    //7 盲区
    vector<RectFValueArray> drivingMissingArea;
    //8 驶入
    vector<RectFValueArray> drivingInArea;
    int piexlType = 0;
    int minTrackDistance = 200;
    int maxTrackDistance = 400;
    double areaRatio = 0.8;
    int failCount1 = 10;
    int failCount2 = 2;
    double piexlByMeterX = 10.245;
    double piexlByMeterY = 7.76;
    double speedMovingAverage = 0.1;
    int minIntersectionArea = 50;
    int roadLength = 100;
    double shakingPixelThreshold = 0.1;
    int stoplineLength = 5;
    int trackInLength = 50;
    int trackOutLength = 20;
    double fusionMinDistance = 130;
    double fusionMaxDistance = 180;
    int maxSpeedByPiexl = 20;
    int carMatchCount = 2;
    double maxCenterDistanceThreshold = 0.5;
    double minCenterDistanceThreshold = 0.3;
    double minAreaThreshold = 0.3;
    double maxAreaThreshold = 0.7;
    double middleAreaThreshold = 0.6;
    double maxStopSpeedThreshold = 0.6;
    double minStopSpeedThreshold = 0.4;

    //9 GPS修正
    vector<CorrectedValueGPS> correctedValueGPSs;


XPACK(O(intersectionName,
        transformMatrix,
        angle,
        startUTMX, startUTMY,
        midLongitude, midLatitude,
        distanceOfStoplineFromRoadcenter,
        selectionCriteria,
        drivingArea, drivingMissingArea, drivingInArea,
        piexlType, minTrackDistance, maxTrackDistance, areaRatio,
        failCount1, failCount2, piexlByMeterX, piexlByMeterY,
        speedMovingAverage, minIntersectionArea, roadLength, shakingPixelThreshold,
        stoplineLength, trackInLength, trackOutLength,
        fusionMinDistance, fusionMaxDistance, maxSpeedByPiexl, carMatchCount,
        maxCenterDistanceThreshold, minCenterDistanceThreshold,
        minAreaThreshold, maxAreaThreshold, middleAreaThreshold,
        maxStopSpeedThreshold, minStopSpeedThreshold));

};


#endif //_MERGESTRUCT_H
