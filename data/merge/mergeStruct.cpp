//
// Created by lining on 6/30/22.
//

#include <valarray>
#include "merge.h"
#include "mergeStruct.h"

const int INF = 0x7FFFFFFF;

void OBJECT_INFO_T2ObjTarget(OBJECT_INFO_T &objectInfoT, ObjTarget &objTarget) {
    objTarget.objID = to_string(objectInfoT.objID);
    objTarget.objCameraID = objectInfoT.cameraID;
    objTarget.objType = objectInfoT.objType;
    objTarget.plates = string(objectInfoT.plate_number);
    objTarget.plateColor = string(objectInfoT.plate_color);
    objTarget.left = objectInfoT.left;
    objTarget.top = objectInfoT.top;
    objTarget.right = objectInfoT.right;
    objTarget.bottom = objectInfoT.bottom;
    objTarget.locationX = objectInfoT.locationX;
    objTarget.locationY = objectInfoT.locationY;
    objTarget.distance = string(objectInfoT.distance);
    objTarget.directionAngle = objectInfoT.directionAngle;
//    objTarget.speed = to_string(objectInfoT.speed);
    objTarget.speedX = objectInfoT.speedX;
    objTarget.speedY = objectInfoT.speedY;
    objTarget.longitude = objectInfoT.longitude;
    objTarget.latitude = objectInfoT.latitude;
}

void ObjTarget2OBJECT_INFO_T(ObjTarget &objTarget, OBJECT_INFO_T &objectInfoT) {

    objectInfoT.objID = atoi(objTarget.objID.c_str());

    objectInfoT.cameraID = objTarget.objCameraID;

    objectInfoT.objType = objTarget.objType;

    bzero(objectInfoT.plate_number, ARRAY_SIZE(objectInfoT.plate_number));
    sprintf(objectInfoT.plate_number, "%s", objTarget.plates.c_str());

    bzero(objectInfoT.plate_color, ARRAY_SIZE(objectInfoT.plate_color));
    sprintf(objectInfoT.plate_color, "%s", objTarget.plateColor.c_str());

    objectInfoT.left = objTarget.left;
    objectInfoT.top = objTarget.top;
    objectInfoT.right = objTarget.right;
    objectInfoT.bottom = objTarget.bottom;
    objectInfoT.locationX = objTarget.locationX;
    objectInfoT.locationY = objTarget.locationY;

    bzero(objectInfoT.distance, ARRAY_SIZE(objectInfoT.distance));
    sprintf(objectInfoT.distance, "%s", objTarget.distance.c_str());

    objectInfoT.directionAngle = objTarget.directionAngle;

//    objectInfoT.speed = atof(objTarget.speed.data());
    objectInfoT.speedX = objTarget.speedX;
    objectInfoT.speedY = objTarget.speedY;

    objectInfoT.longitude = objTarget.longitude;
    objectInfoT.latitude = objTarget.latitude;
//    objectInfoT.carLength = objTarget.carLength;
//    objectInfoT.carFeaturePic = objTarget.carFeaturePic;

}

void OBJECT_INFO_T2OBJECT_INFO_NEW(OBJECT_INFO_T &objectInfoT, OBJECT_INFO_NEW &objectInfoNew) {
    objectInfoNew.objID1 = objectInfoT.objID;
    objectInfoNew.objID2 = -INF;
    objectInfoNew.objID3 = -INF;
    objectInfoNew.objID4 = -INF;
    objectInfoNew.cameraID1 = objectInfoT.cameraID;
    objectInfoNew.cameraID2 = -INF;
    objectInfoNew.cameraID3 = -INF;
    objectInfoNew.cameraID4 = -INF;
    objectInfoNew.showID = objectInfoT.objID;
    objectInfoNew.objType = objectInfoT.objType;

    memcpy(objectInfoNew.plate_number, objectInfoT.plate_number, sizeof(objectInfoNew.plate_number));
    memcpy(objectInfoNew.plate_color, objectInfoT.plate_color, sizeof(objectInfoNew.plate_color));

    objectInfoNew.left = objectInfoT.left;
    objectInfoNew.top = objectInfoT.top;
    objectInfoNew.right = objectInfoT.right;
    objectInfoNew.bottom = objectInfoT.bottom;
    objectInfoNew.locationX = objectInfoT.locationX;
    objectInfoNew.locationY = objectInfoT.locationY;

    memcpy(objectInfoNew.distance, objectInfoT.distance, sizeof(objectInfoNew.distance));
    objectInfoNew.directionAngle = objectInfoT.directionAngle;
    objectInfoNew.speed = sqrt(objectInfoT.speedX * objectInfoT.speedX + objectInfoT.speedY * objectInfoT.speedY);
    objectInfoNew.longitude = objectInfoT.longitude;
    objectInfoNew.latitude = objectInfoT.latitude;
//    objectInfoNew.carLength = objectInfoT.carLength;
//    objectInfoNew.carFeaturePic = objectInfoT.carFeaturePic;
}

void OBJECT_INFO_NEW2ObjMix(OBJECT_INFO_NEW &objectInfoNew, ObjMix &objMix) {
    objMix.objID = to_string(objectInfoNew.showID);
    objMix.objType = objectInfoNew.objType;
    objMix.objColor = 0;
    objMix.angle = objectInfoNew.directionAngle;
    objMix.speed = objectInfoNew.speed;
    objMix.locationX = objectInfoNew.locationX;
    objMix.locationY = objectInfoNew.locationY;
    objMix.longitude = objectInfoNew.longitude;
    objMix.latitude = objectInfoNew.latitude;
    objMix.flagNew = objectInfoNew.flag_new;
    //添加属性
//    objMix.laneCode = objectInfoNew.laneCode;
//    objMix.carLength = objectInfoNew.carLength;
//    objMix.carFeaturePic = objectInfoNew.carFeaturePic;
//    objMix.plates = objectInfoNew.plates;
//    objMix.plateColor = objectInfoNew.plateColor;
//    objMix.carType = objectInfoNew.carType;
}

bool PointF::JsonMarshal(Json::Value &out) {
    out["x"] = this->x;
    out["y"] = this->y;
    return true;
}

bool PointF::JsonUnmarshal(Json::Value in) {
    this->x = in["x"].asDouble();
    this->y = in["y"].asDouble();
    return true;
}

bool PointFArray::JsonMarshal(Json::Value &out) {
    out["index"] = this->index;
    Json::Value points = Json::arrayValue;
    if (!this->points.empty()) {
        for (auto iter: this->points) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                points.append(item);
            }
        }
    } else {
        points.resize(0);
    }
    out["points"] = points;

    return true;
}

bool PointFArray::JsonUnmarshal(Json::Value in) {
    this->index = in["index"].asInt();
    if (in["points"].isArray()) {
        Json::Value points = in["points"];
        for (auto iter: points) {
            PointF item;
            if (item.JsonUnmarshal(iter)) {
                this->points.push_back(item);
            }
        }
    }

    return true;
}


bool DoubleValue::JsonMarshal(Json::Value &out) {
    out["index"] = this->index;
    out["value"] = this->value;
    return true;
}

bool DoubleValue::JsonUnmarshal(Json::Value in) {
    this->index = in["index"].asInt();
    this->value = in["value"].asDouble();
    return true;
}

bool CorrectedValueGPS::JsonMarshal(Json::Value &out) {
    out["index"] = this->index;
    out["latitude"] = this->latitude;
    out["longitude"] = this->longitude;

    return true;
}

bool CorrectedValueGPS::JsonUnmarshal(Json::Value in) {
    this->index = in["index"].asInt();
    this->latitude = in["latitude"].asDouble();
    this->longitude = in["longitude"].asDouble();

    return true;
}

bool AlgorithmParam::JsonMarshal(Json::Value &out) {

    Json::Value H_east = Json::arrayValue;
    if (!this->H_east.empty()) {
        for (auto iter: this->H_east) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                H_east.append(item);
            }
        }
    } else {
        H_east.resize(0);
    }
    out["H_east"] = H_east;

    Json::Value H_north = Json::arrayValue;
    if (!this->H_north.empty()) {
        for (auto iter: this->H_north) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                H_north.append(item);
            }
        }
    } else {
        H_north.resize(0);
    }
    out["H_north"] = H_north;

    Json::Value H_west = Json::arrayValue;
    if (!this->H_west.empty()) {
        for (auto iter: this->H_west) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                H_west.append(item);
            }
        }
    } else {
        H_west.resize(0);
    }
    out["H_west"] = H_west;

    Json::Value H_south = Json::arrayValue;
    if (!this->H_south.empty()) {
        for (auto iter: this->H_south) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                H_south.append(item);
            }
        }
    } else {
        H_south.resize(0);
    }
    out["H_south"] = H_south;
    out["start_utm_x"] = this->start_utm_x;
    out["start_utm_y"] = this->start_utm_y;
    out["crossroad_mid_longitude"] = this->crossroad_mid_longitude;
    out["crossroad_mid_latitude"] = this->crossroad_mid_latitude;
    out["distance_of_east_stopline_from_roadcenter"] = this->distance_of_east_stopline_from_roadcenter;
    out["distance_of_west_stopline_from_roadcenter"] = this->distance_of_west_stopline_from_roadcenter;
    out["distance_of_north_stopline_from_roadcenter"] = this->distance_of_north_stopline_from_roadcenter;
    out["distance_of_south_stopline_from_roadcenter"] = this->distance_of_south_stopline_from_roadcenter;
    out["east_west_selection_criteria"] = this->east_west_selection_criteria;

    Json::Value angle = Json::arrayValue;
    if (!this->angle.empty()) {
        for (auto iter: this->angle) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                angle.append(item);
            }
        }
    } else {
        angle.resize(0);
    }
    out["angle"] = angle;


    Json::Value east_west_driving_area = Json::arrayValue;
    if (!this->east_west_driving_area.empty()) {
        for (auto iter: this->east_west_driving_area) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                east_west_driving_area.append(item);
            }
        }
    } else {
        east_west_driving_area.resize(0);
    }
    out["east_west_driving_area"] = east_west_driving_area;

    Json::Value west_east_driving_area = Json::arrayValue;
    if (!this->west_east_driving_area.empty()) {
        for (auto iter: this->west_east_driving_area) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                west_east_driving_area.append(item);
            }
        }
    } else {
        west_east_driving_area.resize(0);
    }
    out["west_east_driving_area"] = west_east_driving_area;

    Json::Value north_south_driving_area = Json::arrayValue;
    if (!this->north_south_driving_area.empty()) {
        for (auto iter: this->north_south_driving_area) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                north_south_driving_area.append(item);
            }
        }
    } else {
        north_south_driving_area.resize(0);
    }
    out["north_south_driving_area"] = north_south_driving_area;

    Json::Value south_north_driving_area = Json::arrayValue;
    if (!this->south_north_driving_area.empty()) {
        for (auto iter: this->south_north_driving_area) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                south_north_driving_area.append(item);
            }
        }
    } else {
        south_north_driving_area.resize(0);
    }
    out["south_north_driving_area"] = south_north_driving_area;

    out["piexl_type"] = this->piexl_type;
    out["min_track_distance"] = this->min_track_distance;
    out["max_track_distance"] = this->max_track_distance;
    out["area_ratio"] = this->area_ratio;
    out["failCount1"] = this->failCount1;
    out["failCount2"] = this->failCount2;
    out["piexlbymeter_x"] = this->piexlbymeter_x;
    out["piexlbymeter_y"] = this->piexlbymeter_y;
    out["speed_moving_average"] = this->speed_moving_average;
    out["min_intersection_area"] = this->min_intersection_area;
    out["road_length"] = this->road_length;
    out["shaking_pixel_threshold"] = this->shaking_pixel_threshold;
    out["stopline_length"] = this->stopline_length;
    out["track_in_length"] = this->track_in_length;
    out["track_out_length"] = this->track_out_length;
    out["car_match_count"] = this->car_match_count;
    out["max_center_distance_threshold"] = this->max_center_distance_threshold;
    out["min_center_distance_threshold"] = this->min_center_distance_threshold;
    out["min_area_threshold"] = this->min_area_threshold;
    out["max_area_threshold"] = this->max_area_threshold;
    out["middle_area_threshold"] = this->middle_area_threshold;
    out["max_speed_by_piexl"] = this->max_speed_by_piexl;
    out["max_stop_speed_threshold"] = this->max_stop_speed_threshold;
    out["min_stop_speed_threshold"] = this->max_stop_speed_threshold;

    Json::Value correctedValueGPSs = Json::arrayValue;
    if (!this->correctedValueGPSs.empty()) {
        for (auto iter: this->correctedValueGPSs) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                correctedValueGPSs.append(item);
            }
        }
    } else {
        correctedValueGPSs.resize(0);
    }
    out["correctedValueGPSs"] = correctedValueGPSs;


    return true;
}

bool AlgorithmParam::JsonUnmarshal(Json::Value in) {

    if (in["H_east"].isArray()) {
        Json::Value H_east = in["H_east"];
        for (auto iter: H_east) {
            DoubleValue item;
            if (item.JsonUnmarshal(iter)) {
                this->H_east.push_back(item);
            }
        }
    }

    if (in["H_north"].isArray()) {
        Json::Value H_north = in["H_north"];
        for (auto iter: H_north) {
            DoubleValue item;
            if (item.JsonUnmarshal(iter)) {
                this->H_north.push_back(item);
            }
        }
    }

    if (in["H_west"].isArray()) {
        Json::Value H_west = in["H_west"];
        for (auto iter: H_west) {
            DoubleValue item;
            if (item.JsonUnmarshal(iter)) {
                this->H_west.push_back(item);
            }
        }
    }

    if (in["H_south"].isArray()) {
        Json::Value H_south = in["H_south"];
        for (auto iter: H_south) {
            DoubleValue item;
            if (item.JsonUnmarshal(iter)) {
                this->H_south.push_back(item);
            }
        }
    }
    if (in.isMember("start_utm_x")) {
        this->start_utm_x = in["start_utm_x"].asInt();
    }
    if (in.isMember("start_utm_y")) {
        this->start_utm_y = in["start_utm_y"].asInt();
    }
    if (in.isMember("crossroad_mid_longitude")) {
        this->crossroad_mid_longitude = in["crossroad_mid_longitude"].asDouble();
    }
    if (in.isMember("crossroad_mid_latitude")) {
        this->crossroad_mid_latitude = in["crossroad_mid_latitude"].asDouble();
    }
    if (in.isMember("distance_of_east_stopline_from_roadcenter")) {
        this->distance_of_east_stopline_from_roadcenter = in["distance_of_east_stopline_from_roadcenter"].asInt();
    }
    if (in.isMember("distance_of_west_stopline_from_roadcenter")) {
        this->distance_of_west_stopline_from_roadcenter = in["distance_of_west_stopline_from_roadcenter"].asInt();
    }
    if (in.isMember("distance_of_north_stopline_from_roadcenter")) {
        this->distance_of_north_stopline_from_roadcenter = in["distance_of_north_stopline_from_roadcenter"].asInt();
    }
    if (in.isMember("distance_of_south_stopline_from_roadcenter")) {
        this->distance_of_south_stopline_from_roadcenter = in["distance_of_south_stopline_from_roadcenter"].asInt();
    }
    if (in.isMember("east_west_selection_criteria")) {
        this->east_west_selection_criteria = in["east_west_selection_criteria"].asInt();
    }

    if (in["angle"].isArray()) {
        Json::Value angle = in["angle"];
        for (auto iter: angle) {
            DoubleValue item;
            if (item.JsonUnmarshal(iter)) {
                this->angle.push_back(item);
            }
        }
    }

    if (in["east_west_driving_area"].isArray()) {
        Json::Value east_west_driving_area = in["east_west_driving_area"];
        for (auto iter: east_west_driving_area) {
            PointFArray item;
            if (item.JsonUnmarshal(iter)) {
                this->east_west_driving_area.push_back(item);
            }
        }
    }

    if (in["west_east_driving_area"].isArray()) {
        Json::Value west_east_driving_area = in["west_east_driving_area"];
        for (auto iter: west_east_driving_area) {
            PointFArray item;
            if (item.JsonUnmarshal(iter)) {
                this->west_east_driving_area.push_back(item);
            }
        }
    }

    if (in["north_south_driving_area"].isArray()) {
        Json::Value north_south_driving_area = in["north_south_driving_area"];
        for (auto iter: north_south_driving_area) {
            PointFArray item;
            if (item.JsonUnmarshal(iter)) {
                this->north_south_driving_area.push_back(item);
            }
        }
    }

    if (in["south_north_driving_area"].isArray()) {
        Json::Value south_north_driving_area = in["south_north_driving_area"];
        for (auto iter: south_north_driving_area) {
            PointFArray item;
            if (item.JsonUnmarshal(iter)) {
                this->south_north_driving_area.push_back(item);
            }
        }
    }

    if (in.isMember("piexl_type")) {
        this->piexl_type = in["piexl_type"].asInt();
    }
    if (in.isMember("min_track_distance")) {
        this->min_track_distance = in["min_track_distance"].asInt();
    }
    if (in.isMember("max_track_distance")) {
        this->max_track_distance = in["max_track_distance"].asInt();
    }
    if (in.isMember("area_ratio")) {
        this->area_ratio = in["area_ratio"].asDouble();
    }
    if (in.isMember("failCount1")) {
        this->failCount1 = in["failCount1"].asInt();
    }
    if (in.isMember("failCount2")) {
        this->failCount2 = in["failCount2"].asInt();
    }
    if (in.isMember("piexlbymeter_x")) {
        this->piexlbymeter_x = in["piexlbymeter_x"].asDouble();
    }
    if (in.isMember("piexlbymeter_y")) {
        this->piexlbymeter_y = in["piexlbymeter_y"].asDouble();
    }
    if (in.isMember("speed_moving_average")) {
        this->speed_moving_average = in["speed_moving_average"].asDouble();
    }
    if (in.isMember("min_intersection_area")) {
        this->min_intersection_area = in["min_intersection_area"].asInt();
    }
    if (in.isMember("road_length")) {
        this->road_length = in["road_length"].asInt();
    }
    //zone
    if (in.isMember("zone")) {
        this->road_length = in["zone"].asInt();
    }
    if (in.isMember("shaking_pixel_threshold")) {
        this->shaking_pixel_threshold = in["shaking_pixel_threshold"].asDouble();
    }
    if (in.isMember("stopline_length")) {
        this->stopline_length = in["stopline_length"].asInt();
    }
    if (in.isMember("track_in_length")) {
        this->track_in_length = in["track_in_length"].asInt();
    }
    if (in.isMember("track_out_length")) {
        this->track_out_length = in["track_out_length"].asInt();
    }
    if (in.isMember("car_match_count")) {
        this->car_match_count = in["car_match_count"].asInt();
    }
    if (in.isMember("max_center_distance_threshold")) {
        this->max_center_distance_threshold = in["max_center_distance_threshold"].asDouble();
    }
    if (in.isMember("min_center_distance_threshold")) {
        this->min_center_distance_threshold = in["min_center_distance_threshold"].asDouble();
    }
    if (in.isMember("min_area_threshold")) {
        this->min_area_threshold = in["min_area_threshold"].asDouble();
    }
    if (in.isMember("max_area_threshold")) {
        this->max_area_threshold = in["max_area_threshold"].asDouble();
    }
    if (in.isMember("middle_area_threshold")) {
        this->middle_area_threshold = in["middle_area_threshold"].asDouble();
    }
    if (in.isMember("max_speed_by_piexl")) {
        this->max_speed_by_piexl = in["max_speed_by_piexl"].asInt();
    }
    if (in.isMember("max_stop_speed_threshold")) {
        this->max_stop_speed_threshold = in["max_stop_speed_threshold"].asDouble();
    }
    if (in.isMember("min_stop_speed_threshold")) {
        this->min_stop_speed_threshold = in["min_stop_speed_threshold"].asDouble();
    }

    if (in["correctedValueGPSs"].isArray()) {
        Json::Value correctedValueGPSs = in["correctedValueGPSs"];
        for (auto iter: correctedValueGPSs) {
            CorrectedValueGPS item;
            if (item.JsonUnmarshal(iter)) {
                this->correctedValueGPSs.push_back(item);
            }
        }
    }

    return true;
}

void AlgorithmParam::setH_XX(vector<DoubleValue> &H_xx, vector<double> values) {
    for (int i = 0; i < values.size(); i++) {
        DoubleValue dv;
        dv.index = i;
        dv.value = values.at(i);
        H_xx.push_back(dv);
    }
}

void AlgorithmParam::getH_XX(vector<DoubleValue> H_xx, vector<double> &values) {
    //先将H_xx排序
    if (H_xx.size() > 1) {
        std::sort(H_xx.begin(), H_xx.end(), [](DoubleValue a, DoubleValue b) {
            return a.index < b.index;
        });
    }
    for (auto iter: H_xx) {
        values.push_back(iter.value);
    }
}

void AlgorithmParam::setAngle(vector<double> values) {
    for (int i = 0; i < values.size(); i++) {
        DoubleValue dv;
        dv.index = i;
        dv.value = values.at(i);
        this->angle.push_back(dv);
    }
}

void AlgorithmParam::getAngle(vector<double> &values) {
    //先将angle排序
    if (angle.size() > 1) {
        std::sort(angle.begin(), angle.end(), [](DoubleValue a, DoubleValue b) {
            return a.index < b.index;
        });
    }

    for (auto iter: angle) {
        values.push_back(iter.value);
    }
}

void AlgorithmParam::setXX_driving_are(vector<PointFArray> &xx, vector<vector<PointF>> values) {
    for (int i = 0; i < values.size(); i++) {
        PointFArray pointFArray;
        pointFArray.index = i;
        auto iter = values.at(i);
        for (auto iter1: iter) {
            pointFArray.points.push_back(iter1);
        }
        xx.push_back(pointFArray);
    }
}

void AlgorithmParam::getXX_driving_are(vector<PointFArray> xx, vector<vector<PointF>> &values) {
    //先将xx排序
    if (xx.size() > 1) {
        std::sort(xx.begin(), xx.end(), [](PointFArray a, PointFArray b) {
            return a.index < b.index;
        });
    }

    for (auto iter: xx) {
        vector<PointF> v_pointF;
        vector<PointF>().swap(v_pointF);
        for (auto iter1: iter.points) {
            v_pointF.push_back(iter1);
        }
        values.push_back(v_pointF);
    }
}

//void AlgorithmParam::getFromENW_ImageUnifiedParamsENW(class ImageUnifiedParamsENW in) {
//    vector<double> v_double;
//    //H_east
//    v_double.clear();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            v_double.push_back(in.H_east[i][j]);
//        }
//    }
//    this->setH_XX(this->H_east, v_double);
//
//    //H_north
//    v_double.clear();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            v_double.push_back(in.H_north[i][j]);
//        }
//    }
//    this->setH_XX(this->H_north, v_double);
//
//    //H_west
//    v_double.clear();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            v_double.push_back(in.H_west[i][j]);
//        }
//    }
//    this->setH_XX(this->H_west, v_double);
//
//    //H_south
//    v_double.clear();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            v_double.push_back(in.H_south[i][j]);
//        }
//    }
//    this->setH_XX(this->H_south, v_double);
//
//    //每个路口的中心经纬度
//    this->crossroad_mid_longitude = in.crossroad_mid_longitude;//经度
//    this->crossroad_mid_latitude = in.crossroad_mid_latitude;//纬度
//    //路口中心距离每侧停止线的距离（m）
//    this->distance_of_east_stopline_from_roadcenter = in.diatance_of_east_stopline_from_roadcenter;
//    this->distance_of_west_stopline_from_roadcenter = in.diatance_of_west_stopline_from_roadcenter;
//    this->distance_of_north_stopline_from_roadcenter = in.diatance_of_north_stopline_from_roadcenter;
//    this->distance_of_south_stopline_from_roadcenter = in.diatance_of_south_stopline_from_roadcenter;
//
//    //每侧路口的倾斜角度
//    v_double.clear();
//    for (auto iter: in.angle) {
//        v_double.push_back(iter);
//    }
//    this->setAngle(v_double);
//
//
//    vector<vector<PointF>> v_v_points;
//    //east_west_driving_area;   //东向西行驶的车辆
//    v_v_points.clear();
//    for (auto iter: in.east_west_driving_area) {
//        vector<PointF> v_points;
//        for (auto iter1: iter) {
//            PointF point;
//            point.x = iter1.x;
//            point.y = iter1.y;
//            v_points.push_back(point);
//        }
//        v_v_points.push_back(v_points);
//    }
//
//    this->setXX_driving_are(this->east_west_driving_area, v_v_points);
//    //west_east_driving_area;   //西向东行驶的车辆
//    v_v_points.clear();
//    for (auto iter: in.west_east_driving_area) {
//        vector<PointF> v_points;
//        for (auto iter1: iter) {
//            PointF point;
//            point.x = iter1.x;
//            point.y = iter1.y;
//            v_points.push_back(point);
//        }
//        v_v_points.push_back(v_points);
//    }
//
//    this->setXX_driving_are(this->west_east_driving_area, v_v_points);
//
//    //north_south_driving_area; //北向南行驶的车辆
//    v_v_points.clear();
//    for (auto iter: in.north_south_driving_area) {
//        vector<PointF> v_points;
//        for (auto iter1: iter) {
//            PointF point;
//            point.x = iter1.x;
//            point.y = iter1.y;
//            v_points.push_back(point);
//        }
//        v_v_points.push_back(v_points);
//    }
//
//    this->setXX_driving_are(this->north_south_driving_area, v_v_points);
//
//    //south_north_driving_area; //南向北行驶的车辆
//    v_v_points.clear();
//    for (auto iter: in.south_north_driving_area) {
//        vector<PointF> v_points;
//        for (auto iter1: iter) {
//            PointF point;
//            point.x = iter1.x;
//            point.y = iter1.y;
//            v_points.push_back(point);
//        }
//        v_v_points.push_back(v_points);
//    }
//
//    this->setXX_driving_are(this->south_north_driving_area, v_v_points);
//
//    this->piexl_type = in.piexl_type;
//    this->min_track_distance = in.min_track_distance;
//    this->max_track_distance = in.max_track_distance;
//    this->failCount1 = in.failCount1;
//    this->failCount2 = in.failCount2;
//    this->piexlbymeter_x = in.piexlbymeter_x;
//    this->piexlbymeter_y = in.piexlbymeter_y;
//    this->road_length = in.road_length;
//    this->max_stop_speed_threshold = in.max_stop_speed_threshold;
//    this->shaking_pixel_threshold = in.shaking_pixel_threshold;
//    this->stopline_length = in.stopline_length;
//    this->track_in_length = in.track_in_length;
//    this->track_out_length = in.track_out_length;
//    this->max_speed_by_piexl = in.max_speed_by_piexl;
//    this->car_match_count = in.car_match_count;
//    this->max_center_distance_threshold = in.max_center_distance_threshold;
//    this->min_center_distance_threshold = in.min_center_distance_threshold;
//    this->min_area_threshold = in.min_area_threshold;
//    this->max_area_threshold = in.max_area_threshold;
//    this->middle_area_threshold = in.middle_area_threshold;
//}
//
//void AlgorithmParam::setToENW_ImageUnifiedParamsENW(class ImageUnifiedParamsENW &out) {
//    vector<double> v_double;
//    //H_east
//    v_double.clear();
//    this->getH_XX(this->H_east, v_double);
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            out.H_east[i][j] = v_double.at(i * 3 + j);
//        }
//    }
//
//    //H_north
//    v_double.clear();
//    this->getH_XX(this->H_north, v_double);
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            out.H_north[i][j] = v_double.at(i * 3 + j);
//        }
//    }
//
//    //H_west
//    v_double.clear();
//    this->getH_XX(this->H_west, v_double);
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            out.H_west[i][j] = v_double.at(i * 3 + j);
//        }
//    }
//
//    //H_south
//    v_double.clear();
//    this->getH_XX(this->H_south, v_double);
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < 3; j++) {
//            out.H_south[i][j] = v_double.at(i * 3 + j);
//        }
//    }
//
//    //每个路口的中心经纬度
//    out.crossroad_mid_longitude = this->crossroad_mid_longitude;//经度
//    out.crossroad_mid_latitude = this->crossroad_mid_latitude;//纬度
//    //路口中心距离每侧停止线的距离（m）
//    out.diatance_of_east_stopline_from_roadcenter = this->distance_of_east_stopline_from_roadcenter;
//    out.diatance_of_west_stopline_from_roadcenter = this->distance_of_west_stopline_from_roadcenter;
//    out.diatance_of_north_stopline_from_roadcenter = this->distance_of_north_stopline_from_roadcenter;
//    out.diatance_of_south_stopline_from_roadcenter = this->distance_of_south_stopline_from_roadcenter;
//
//    //每侧路口的倾斜角度
//    v_double.clear();
//    this->getAngle(v_double);
//    for (int i = 0; i < v_double.size(); i++) {
//        out.angle[i] = v_double.at(i);
//    }
//
//    vector<vector<PointF>> v_v_points;
//    //east_west_driving_area;   //东向西行驶的车辆
//    v_v_points.clear();
//    this->getXX_driving_are(this->east_west_driving_area, v_v_points);
//    for (auto iter: v_v_points) {
//        vector<Point2f> v_point2f;
//        for (auto iter1: iter) {
//            Point2f point2f;
//            point2f.x = iter1.x;
//            point2f.y = iter1.y;
//            v_point2f.push_back(point2f);
//        }
//        out.east_west_driving_area.push_back(v_point2f);
//    }
//
//    //west_east_driving_area;   //西向东行驶的车辆
//    v_v_points.clear();
//    this->getXX_driving_are(this->west_east_driving_area, v_v_points);
//    for (auto iter: v_v_points) {
//        vector<Point2f> v_point2f;
//        for (auto iter1: iter) {
//            Point2f point2f;
//            point2f.x = iter1.x;
//            point2f.y = iter1.y;
//            v_point2f.push_back(point2f);
//        }
//        out.west_east_driving_area.push_back(v_point2f);
//    }
//
//
//    //north_south_driving_area; //北向南行驶的车辆
//    v_v_points.clear();
//    this->getXX_driving_are(this->north_south_driving_area, v_v_points);
//    for (auto iter: v_v_points) {
//        vector<Point2f> v_point2f;
//        for (auto iter1: iter) {
//            Point2f point2f;
//            point2f.x = iter1.x;
//            point2f.y = iter1.y;
//            v_point2f.push_back(point2f);
//        }
//        out.north_south_driving_area.push_back(v_point2f);
//    }
//
//
//    //south_north_driving_area; //南向北行驶的车辆
//    v_v_points.clear();
//    this->getXX_driving_are(this->south_north_driving_area, v_v_points);
//    for (auto iter: v_v_points) {
//        vector<Point2f> v_point2f;
//        for (auto iter1: iter) {
//            Point2f point2f;
//            point2f.x = iter1.x;
//            point2f.y = iter1.y;
//            v_point2f.push_back(point2f);
//        }
//        out.south_north_driving_area.push_back(v_point2f);
//    }
//
//
//    out.piexl_type = this->piexl_type;
//    out.min_track_distance = this->min_track_distance;
//    out.max_track_distance = this->max_track_distance;
//    out.failCount1 = this->failCount1;
//    out.failCount2 = this->failCount2;
//    out.piexlbymeter_x = this->piexlbymeter_x;
//    out.piexlbymeter_y = this->piexlbymeter_y;
//    out.road_length = this->road_length;
//    out.max_stop_speed_threshold = this->max_stop_speed_threshold;
//    out.shaking_pixel_threshold = this->shaking_pixel_threshold;
//    out.stopline_length = this->stopline_length;
//    out.track_in_length = this->track_in_length;
//    out.track_out_length = this->track_out_length;
//    out.max_speed_by_piexl = this->max_speed_by_piexl;
//    out.car_match_count = this->car_match_count;
//    out.max_center_distance_threshold = this->max_center_distance_threshold;
//    out.min_center_distance_threshold = this->min_center_distance_threshold;
//    out.min_area_threshold = this->min_area_threshold;
//    out.max_area_threshold = this->max_area_threshold;
//    out.middle_area_threshold = this->middle_area_threshold;
//}
//

