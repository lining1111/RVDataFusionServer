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

/**
 * 基础类说明
 * 基础类分类，依据包裹的层级关系进行分类，基础0类被基础1类包裹，基础1类被基础2类包裹，以此类推
 * 包裹的含义：A包裹B，说明A内有一个vector<B>,同时可能伴随一个int型枚举数值(如 direction,index等)
 *
 */

//----基础0类-----//

//angleItem
class AngleItem {
public:
    int faceDirection;
    double value;

    AngleItem() {

    }

    AngleItem(int faceDirection, double value) : faceDirection(faceDirection), value(value) {

    }

XPACK(O(faceDirection, value));
};

//浮点数+index
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

class Distance {
public:
    int faceDirection;
    double value;

    Distance() {

    }

    Distance(int faceDirection, double value) : faceDirection(faceDirection), value(value) {

    }

XPACK(O(faceDirection, value));
};

//整型+index
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

//点
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

//gps+index
class CorrectedValueGPS {
public:
    int faceDirection = 0;
    double latitude = 0.0;
    double longitude = 0.0;

    CorrectedValueGPS() {

    }

    CorrectedValueGPS(int index, double latitude, double longitude)
            : faceDirection(index), latitude(latitude), longitude(longitude) {

    }

XPACK(O(faceDirection, latitude, longitude));
};

//矩形
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

//矩形+index
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

//阈值
class Threshold {
public:
    double min = 0.0;
    double max = 0.0;
    double mid = 0.0;

    Threshold() {

    }

    Threshold(double min, double max, double mid) : min(min), max(max), mid(mid) {

    }

XPACK(O(min, max, mid));
};

//midCar
class MidCar {
public:
    double rangeLeftX = 0;
    double rangeRightX = 0;
    double mergeWithLeftcarLeftx = 0;
    double mergeWithRightcarRightx = 0;

    MidCar() {

    }

    MidCar(int rangeLeftX, int rangeRightX, int mergeWithLeftcarLeftx, int mergeWithRightcarRightx)
            : rangeLeftX(rangeLeftX), rangeRightX(rangeRightX),
              mergeWithLeftcarLeftx(mergeWithLeftcarLeftx), mergeWithRightcarRightx(mergeWithRightcarRightx) {

    }

XPACK(O(rangeLeftX, rangeRightX, mergeWithLeftcarLeftx, mergeWithRightcarRightx));
};

//TrackDistanceRegionDivision
class TrackDistanceRegionDivision {
public:
    int faceDirection = 0;
    PointF in;
    PointF out;
    int carRangeUpX = 0;
    int carRangeBottomY = 0;

    TrackDistanceRegionDivision() {

    }

    TrackDistanceRegionDivision(int direction, PointF in, PointF out, int carRangeUpX, int carRangeBottomY)
            : faceDirection(direction), in(in), out(out), carRangeUpX(carRangeUpX), carRangeBottomY(carRangeBottomY) {

    }

XPACK(O(faceDirection, in, out, carRangeUpX, carRangeBottomY));
};

//-----基础1类---//

//点数组+index
class PointFArray {
public:
    int index = 0;
    vector<PointF> value;

    PointFArray() {

    }

XPACK(O(index, value));

};

//矩形数组+index
class DrivingAreaX {
public:
    int flowDirection = 0;
    vector<RectFValue> value;

    DrivingAreaX() {

    }

XPACK(O(flowDirection, value));

//将数组设置到类内变量
    void set(int index, vector<RectF> values) {
        this->flowDirection = index;
        this->value.clear();
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
        index = this->flowDirection;

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


//-----基础2类------//
//drivingArea Item
class DrivingAreaItem {
public:
    int flowDirection = 0;
    vector<PointFArray> value;

    DrivingAreaItem() {

    }

XPACK(O(flowDirection, value));

    void set(int index, vector<vector<PointF>> values) {
        this->flowDirection = index;
        this->value.clear();
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
        this->flowDirection = index;
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

//3X3转换矩阵 item
class TransMatrixItem {
public:
    int faceDirection = 0;
    vector<DoubleValue> value;

    TransMatrixItem() {

    }

XPACK(O(faceDirection, value));

    //将数组设置到类内变量
    void set(int index, vector<double> values) {
        this->faceDirection = index;
        this->value.clear();
        for (int i = 0; i < values.size(); i++) {
            DoubleValue dv;
            dv.index = i;
            dv.value = values.at(i);
            this->value.push_back(dv);
        }
    };

    //将类内变量设置到数组
    void get(int &index, vector<double> &values) {
        index = this->faceDirection;
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

//----基础3类-----//
class AlgorithmParam {
public:
    //0 路口名称
    string intersectionName;
    //1 路口转换矩阵
    vector<TransMatrixItem> transformMatrix;//转换矩阵

    //2 路口的倾斜角度
    vector<AngleItem> angle;

    //3 路口中心距离每侧停止线的距离（m）
    vector<Distance> distanceOfStopLineFromRoadCenter;

    //4 驶入区域
    vector<DrivingAreaItem> drivingArea;
    //5 盲区
    vector<DrivingAreaX> drivingMissingArea;
    //6 驶入
    vector<DrivingAreaX> drivingInArea;
    //7 GPS修正
    vector<CorrectedValueGPS> correctedValueGPSs;

    int startUTMX = 325235;
    int startUTMY = 4512985;
    double midLongitude = 114.8901745;//经度
    double midLatitude = 40.7745085;//纬度
    int pixelType = 1;
    int minTrackDistance = 80;
    int maxTrackDistance = 120;
    double areaRatio = 0.8;
    int failCount1 = 5;
    int failCount2 = 5;
    int imageWidth = 1920;
    int imageHeight = 1080;
    double pixelByMeterX = 10.245;
    double pixelByMeterY = 7.76;
    double speedMovingAverage = 0.3;
    double minIntersectionArea = 50.0;
    int roadLength = 300;
    double shakingPixelThreshold = 0.5;
    int stopLineLength = 5;
    int trackInLength = 0;
    int trackOutLength = 20;
    int intersectionsNumber = 4;
    int maxSpeedByPixel = 30;
    int carMatchCount = 3;
    Threshold centerDistanceThreshold;
    Threshold areaThreshold;
    Threshold stopSpeedThreshold;
    int selectionCriteria = 1;
    MidCar midCar;
//    vector<TrackDistanceRegionDivision> trackDistanceRegionDivisionList;
    int zone = 50;
    double maxMatchDistance = 2.5;
    double minSize = 25.0;
    double distanceThresh = 4.0;
    double maxRadarInvisiblePeriod = 0.5;
    double maxCameraInvisiblePeriod = 0.5;
    double fusionMinDistance = 60.0;
    double fusionMaxDistance = 100.0;
    double associateDistanceCamera = 20.0;
    double associateDistanceRadar = 20.0;
    string reserve1;
    string reserve2;
    string reserve3;
    string reserve4;
    string reserve5;
    string reserve6;
    string reserve7;
    string reserve8;
    string reserve9;
    string reserve10;
    string reserve11;
    string reserve12;
    string reserve13;
    string reserve14;
    string reserve15;
    vector<string> selectedErvGuids;

    AlgorithmParam() {
        centerDistanceThreshold.max = 0.4;
        centerDistanceThreshold.min = 0.2;

        areaThreshold.max = 0.3;
        areaThreshold.min = 0.1;
        areaThreshold.mid = 0.2;

        stopSpeedThreshold.max = 2.0;
        stopSpeedThreshold.min = 0.4;

    }

XPACK(O(intersectionName,
        transformMatrix,
        angle,
        startUTMX, startUTMY,
        midLongitude, midLatitude,
        distanceOfStopLineFromRoadCenter,
        selectionCriteria,
        drivingArea, drivingMissingArea, drivingInArea,
        pixelType, minTrackDistance, maxTrackDistance, areaRatio,
        failCount1, failCount2, imageWidth, imageHeight, pixelByMeterX, pixelByMeterY,
        speedMovingAverage, minIntersectionArea, roadLength, shakingPixelThreshold,
        stopLineLength, trackInLength, trackOutLength,intersectionsNumber,
        fusionMinDistance, fusionMaxDistance, maxSpeedByPixel, carMatchCount,
        centerDistanceThreshold, areaThreshold, stopSpeedThreshold,
        correctedValueGPSs, zone, midCar, maxMatchDistance, minSize,
        distanceThresh, maxRadarInvisiblePeriod, maxCameraInvisiblePeriod,
        associateDistanceCamera, associateDistanceRadar,
        reserve1, reserve2, reserve3, reserve4, reserve5, reserve6, reserve7, reserve8, reserve9, reserve10,
        reserve11, reserve12, reserve13, reserve14, reserve15,
        selectedErvGuids));

};


#endif //_MERGESTRUCT_H
