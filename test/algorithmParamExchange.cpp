//
// Created by lining on 2023/10/25.
//

#include "AlgorithmParamOld.h"
#include "lib/AlgorithmParam.h"
#include <gflags/gflags.h>
#include <xpack/json.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <sstream>

using namespace xpack;


string readFile(const string path) {
    //打开文件
    std::ifstream ifs;
    ifs.open(path);
    if (!ifs.is_open()) {
        fmt::print("打开文件失败:{}\n", path);
        return "";
    } else {
        std::stringstream buf;
        buf << ifs.rdbuf();
        std::string content(buf.str());
        ifs.close();
        return content;
    }
}

int writeFile(const string path, const string content) {
    //写入文件
    ofstream file;
    file.open(path, ios::trunc);
    if (file.is_open()) {
        file << content;
        file.flush();
        file.close();
        return 0;
    } else {
        fmt::print("写入文件失败:{}\n", path);
        return -1;
    }
}

//方向枚举0=北,1=东北,2=东,3=东南,4=南,5=西南,6=西,7=西北
enum Direction_Algorithm_face {
    Unknown = 0xff,//未知
    North = 0,//北
    Northeast = 1,//东北
    East = 2,//东
    Southeast = 3,//东南
    South = 4,//南
    Southwest = 5,//西南
    West = 6,//西
    Northwest = 7,//西北
};

typedef struct {
    int oldIndex;
    int newIndex;
} IndexPair;

void oldParam2newParam(AlgorithmParamOld::AlgorithmParam oldParam, AlgorithmParam &newParam) {

    //1.转换矩阵
    // H_east
    {
        TransMatrixItem item;
        item.faceDirection = East;
        for (auto iter: oldParam.H_east) {
            DoubleValue item1;
            item1.index = iter.index;
            item1.value = iter.value;
            item.value.push_back(item1);
        }
        newParam.transformMatrix.push_back(item);
    }
    // H_north
    {
        TransMatrixItem item;
        item.faceDirection = North;
        for (auto iter: oldParam.H_north) {
            DoubleValue item1;
            item1.index = iter.index;
            item1.value = iter.value;
            item.value.push_back(item1);
        }
        newParam.transformMatrix.push_back(item);
    }

    // H_west
    {
        TransMatrixItem item;
        item.faceDirection = West;
        for (auto iter: oldParam.H_west) {
            DoubleValue item1;
            item1.index = iter.index;
            item1.value = iter.value;
            item.value.push_back(item1);
        }
        newParam.transformMatrix.push_back(item);
    }

    // H_south
    {
        TransMatrixItem item;
        item.faceDirection = South;
        for (auto iter: oldParam.H_south) {
            DoubleValue item1;
            item1.index = iter.index;
            item1.value = iter.value;
            item.value.push_back(item1);
        }
        newParam.transformMatrix.push_back(item);
    }

    //2.路口的倾斜角度
    {
        IndexPair pair[4] = {
                {.oldIndex = 0, .newIndex = East},
                {.oldIndex = 1, .newIndex = North},
                {.oldIndex = 2, .newIndex = West},
                {.oldIndex = 3, .newIndex = South},
        };
        for (auto iter: oldParam.angle) {
            AngleItem item;
            int faceDirection = -1;
            for (auto iter1: pair) {
                if (iter.index == iter1.oldIndex) {
                    faceDirection = iter1.newIndex;
                    break;
                }
            }
            if (faceDirection == -1) {
                fmt::print("angle : faceDirection is -1,old index:{}\n", iter.index);
            } else {
                item.faceDirection = faceDirection;
                item.value = iter.value;
                newParam.angle.push_back(item);
            }
        }
    }

    //3.路口中心距离每侧停止线的距离（m）
    //east
    {
        Distance item;
        item.faceDirection = East;
        item.value = oldParam.distance_of_east_stopline_from_roadcenter;
        newParam.distanceOfStoplineFromRoadcenter.push_back(item);
    }
    //west
    {
        Distance item;
        item.faceDirection = West;
        item.value = oldParam.distance_of_west_stopline_from_roadcenter;
        newParam.distanceOfStoplineFromRoadcenter.push_back(item);
    }
    //north
    {
        Distance item;
        item.faceDirection = North;
        item.value = oldParam.distance_of_north_stopline_from_roadcenter;
        newParam.distanceOfStoplineFromRoadcenter.push_back(item);
    }
    //south
    {
        Distance item;
        item.faceDirection = South;
        item.value = oldParam.distance_of_south_stopline_from_roadcenter;
        newParam.distanceOfStoplineFromRoadcenter.push_back(item);
    }

    //4.驶入区域
    //west
    {
        DrivingAreaItem item;
        item.flowDirection = West;
        auto src = &oldParam.east_west_driving_area;
        for (int i = 0; i < src->size(); i++) {
            PointFArray item1;
            item1.index = src->at(i).index;
            for (auto iter: src->at(i).points) {
                PointF item2;
                item2.x = iter.x;
                item2.y = iter.y;
                item1.value.push_back(item2);
            }
            item.value.push_back(item1);
        }
        newParam.drivingArea.push_back(item);
    }
    //east
    {
        DrivingAreaItem item;
        item.flowDirection = East;
        auto src = &oldParam.west_east_driving_area;
        for (int i = 0; i < src->size(); i++) {
            PointFArray item1;
            item1.index = src->at(i).index;
            for (auto iter: src->at(i).points) {
                PointF item2;
                item2.x = iter.x;
                item2.y = iter.y;
                item1.value.push_back(item2);
            }
            item.value.push_back(item1);
        }
        newParam.drivingArea.push_back(item);
    }
    //south
    {
        DrivingAreaItem item;
        item.flowDirection = South;
        auto src = &oldParam.north_south_driving_area;
        for (int i = 0; i < src->size(); i++) {
            PointFArray item1;
            item1.index = src->at(i).index;
            for (auto iter: src->at(i).points) {
                PointF item2;
                item2.x = iter.x;
                item2.y = iter.y;
                item1.value.push_back(item2);
            }
            item.value.push_back(item1);
        }
        newParam.drivingArea.push_back(item);
    }
    //north
    {
        DrivingAreaItem item;
        item.flowDirection = North;
        auto src = &oldParam.south_north_driving_area;
        for (int i = 0; i < src->size(); i++) {
            PointFArray item1;
            item1.index = src->at(i).index;
            for (auto iter: src->at(i).points) {
                PointF item2;
                item2.x = iter.x;
                item2.y = iter.y;
                item1.value.push_back(item2);
            }
            item.value.push_back(item1);
        }
        newParam.drivingArea.push_back(item);
    }

    //5.盲区
    //north
    {
        DrivingAreaX item;
        item.flowDirection = North;
        auto src = &oldParam.east_north_driving_missing_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingMissingArea.push_back(item);
    }
    //west
    {
        DrivingAreaX item;
        item.flowDirection = West;
        auto src = &oldParam.north_west_driving_missing_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingMissingArea.push_back(item);
    }
    //south
    {
        DrivingAreaX item;
        item.flowDirection = South;
        auto src = &oldParam.west_south_driving_missing_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingMissingArea.push_back(item);
    }
    //east
    {
        DrivingAreaX item;
        item.flowDirection = East;
        auto src = &oldParam.south_east_driving_missing_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingMissingArea.push_back(item);
    }

    //6.驶入
    //north
    {
        DrivingAreaX item;
        item.flowDirection = North;
        auto src = &oldParam.east_north_driving_in_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingInArea.push_back(item);
    }
    //west
    {
        DrivingAreaX item;
        item.flowDirection = West;
        auto src = &oldParam.north_west_driving_in_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingInArea.push_back(item);
    }
    //south
    {
        DrivingAreaX item;
        item.flowDirection = South;
        auto src = &oldParam.west_south_driving_in_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingInArea.push_back(item);
    }
    //east
    {
        DrivingAreaX item;
        item.flowDirection = East;
        auto src = &oldParam.south_east_driving_in_area;
        for (auto iter: *src) {
            RectFValue item1;
            item1.index = iter.index;
            item1.x = iter.x;
            item1.y = iter.y;
            item1.width = iter.width;
            item1.height = iter.height;
            item.value.push_back(item1);
        }
        newParam.drivingInArea.push_back(item);
    }

    //7.GPS修正
    {
        //索引新旧转换
        IndexPair pair[4] = {
                {.oldIndex = 0, .newIndex = East},
                {.oldIndex = 1, .newIndex = North},
                {.oldIndex = 2, .newIndex = West},
                {.oldIndex = 3, .newIndex = South},
        };
        for (auto iter: oldParam.correctedValueGPSs) {
            CorrectedValueGPS item;
            int faceDirection = -1;
            for (auto iter1: pair) {
                if (iter.index == iter1.oldIndex) {
                    faceDirection = iter1.newIndex;
                    break;
                }
            }
            if (faceDirection == -1) {
                fmt::print("GPS faceDirection error,old index:{}\n", iter.index);
            } else {
                item.faceDirection = faceDirection;
                item.longitude = iter.longitude;
                item.latitude = iter.latitude;
                newParam.correctedValueGPSs.push_back(item);
            }
        }
    }
    //8.杂项
    newParam.startUTMX = oldParam.start_utm_x;
    newParam.startUTMY = oldParam.start_utm_y;
    newParam.midLongitude = oldParam.crossroad_mid_longitude;
    newParam.midLatitude = oldParam.crossroad_mid_latitude;
    newParam.piexlType = oldParam.piexl_type;
    newParam.minTrackDistance = oldParam.min_track_distance;
    newParam.maxTrackDistance = oldParam.max_track_distance;
    newParam.areaRatio = oldParam.area_ratio;
    newParam.failCount1 = oldParam.failCount1;
    newParam.failCount2 = oldParam.failCount2;
    newParam.piexlByMeterX = oldParam.piexlbymeter_x;
    newParam.piexlByMeterY = oldParam.piexlbymeter_y;
    newParam.speedMovingAverage = oldParam.speed_moving_average;
    newParam.minIntersectionArea = oldParam.min_intersection_area;
    newParam.roadLength = oldParam.road_length;
    newParam.shakingPixelThreshold = oldParam.shaking_pixel_threshold;
    newParam.stoplineLength = oldParam.stopline_length;
    newParam.trackInLength = oldParam.track_in_length;
    newParam.trackOutLength = oldParam.track_out_length;
    newParam.maxSpeedByPiexl = oldParam.max_speed_by_piexl;
    newParam.carMatchCount = oldParam.car_match_count;

    newParam.centerDistanceThreshold.max = oldParam.max_center_distance_threshold;
    newParam.centerDistanceThreshold.min = oldParam.min_center_distance_threshold;

    newParam.areaThreshold.max = oldParam.max_area_threshold;
    newParam.areaThreshold.min = oldParam.min_area_threshold;
    newParam.areaThreshold.mid = oldParam.middle_area_threshold;

    newParam.stopSpeedThreshold.max = oldParam.max_stop_speed_threshold;
    newParam.stopSpeedThreshold.min = oldParam.min_stop_speed_threshold;

    newParam.selectionCriteria = oldParam.east_west_selection_criteria;
    //newParam.midCar?

    //trackDistanceRegionDivisionList?

    //newParam.zone?
    //newParam.maxMatchDistance?
    //newParam.minSize?
    //newParam.distanceThresh?
    //newParam.maxRadarInvisiblePeriod?
    //newParam.maxCameraInvisiblePeriod?
    newParam.fusionMinDistance = oldParam.fusion_min_distance;
    newParam.fusionMaxDistance = oldParam.fusion_max_distance;
    //newParam.associateDistanceCamera?
    //newParam.associateDistanceRadar?
}

DEFINE_string(oldPath, "./old.json", "旧版json位置，默认 old.json");
DEFINE_string(newPath, "./new.json", "新版json位置，默认 new.json");
DEFINE_string(roadName, "roadName", "路口名称，默认 roadName");

int main(int argc, char **argv) {

    gflags::ParseCommandLineFlags(&argc, &argv, true);

    fmt::print("旧版路径: {}\n", FLAGS_oldPath);
    fmt::print("新版路径: {}\n", FLAGS_newPath);

    fmt::print("1.读取旧版文件内容到结构体\n");
    AlgorithmParamOld::AlgorithmParam oldParam;

    string oldContent = readFile(FLAGS_oldPath);
    if (oldContent.empty()) {
        return -1;
    } else {
        try {
            json::decode(oldContent, oldParam);
        } catch (std::exception &e) {
            fmt::print("json解析失败:{}\n", string(e.what()));
            return -1;
        }
    }

    fmt::print("2.将旧版内容转换到新版\n");
    AlgorithmParam newParam;
    newParam.intersectionName = FLAGS_roadName;
    oldParam2newParam(oldParam, newParam);

    fmt::print("3.写入新版文件\n");
    string newContent = json::encode(newParam);
    writeFile(FLAGS_newPath, newContent);
    string webUrl = R"(https://www.sojson.com/)";
    fmt::print("新版文件内容json未格式化，请手动格式化，比如{}\n", webUrl);
    return 0;
}

