//
// Created by lining on 2023/7/5.
//

#include "DataUnitUtilty.h"
#include <glog/logging.h>
#include "common/config.h"
#include "os/os.h"
#include <sys/stat.h>
#include <fstream>
#include <errno.h>
#include <dirent.h>


void SaveDataIn(vector<OBJECT_INFO_T> data, uint64_t timestamp, string path) {
    os::CreatePath(path);
    //存输出数据到文件
    string fileName = path + to_string(timestamp) + ".txt";
    std::ofstream inDataFile;
    inDataFile.open(fileName);
    string content;
    for (int i = 0; i < data.size(); i++) {
        //objSourceType,cameraID,radarID,speedX,speedY,left,top,right,bottom,locationX,locationY,directionAngle,longitude,latitude,objType,laneCode,carLength,carFeaturePic,plates,plateColor
        string split = ",";
        auto iter = data.at(i);
        content += to_string(iter.objSourceType);
        content += split;
        content += to_string(iter.cameraID);
        content += split;
        content += to_string(iter.radarID);
        content += split;
        content += to_string(iter.speedX);
        content += split;
        content += to_string(iter.speedY);
        content += split;
        content += to_string(iter.left);
        content += split;
        content += to_string(iter.top);
        content += split;
        content += to_string(iter.right);
        content += split;
        content += to_string(iter.bottom);
        content += split;
        content += to_string(iter.locationX);
        content += split;
        content += to_string(iter.locationY);
        content += split;
        content += to_string(iter.directionAngle);
        content += split;
        content += to_string(iter.longitude);
        content += split;
        content += to_string(iter.latitude);
        content += split;
        content += to_string(iter.objType);
        content += split;
        content += iter.laneCode;
        content += split;
        content += to_string(iter.carLength);
        content += split;
        content += to_string(iter.carFeaturePic.size());
        content += split;
        content += iter.plates;
        content += split;
        content += iter.plateColor;
        content += "\n";

    }
    if (inDataFile.is_open()) {
//            printf("out:%s\n", content.c_str());
        inDataFile.write((const char *) content.c_str(), content.size());
        inDataFile.flush();
        inDataFile.close();
    } else {
//          Error("打开文件失败:%s", inDataFileNameO.c_str());
    }
}


void SaveDataOut(vector<OBJECT_INFO_NEW> data, uint64_t timestamp, string path) {
    os::CreatePath(path);
    //存输出数据到文件
    string fileName = path + to_string(timestamp) + ".txt";
    std::ofstream inDataFile;
    inDataFile.open(fileName);
    string content;
    for (int i = 0; i < data.size(); i++) {
        //showID,speed,speedX,speedY,left,top,right,bottom,locationX,locationY,directionAngle,longitude,latitude,objType,laneCode,carLength,carFeaturePic,plates,plateColor
        string split = ",";
        auto iter = data.at(i);
        content += to_string(iter.showID);
        content += split;
        content += to_string(iter.speed);
        content += split;
        content += to_string(iter.speedX);
        content += split;
        content += to_string(iter.speedY);
        content += split;
        content += to_string(iter.left);
        content += split;
        content += to_string(iter.top);
        content += split;
        content += to_string(iter.right);
        content += split;
        content += to_string(iter.bottom);
        content += split;
        content += to_string(iter.locationX);
        content += split;
        content += to_string(iter.locationY);
        content += split;
        content += to_string(iter.directionAngle);
        content += split;
        content += to_string(iter.longitude);
        content += split;
        content += to_string(iter.latitude);
        content += split;
        content += to_string(iter.objType);
        content += split;
        content += iter.laneCode;
        content += split;
        content += to_string(iter.carLength);
        content += split;
        content += to_string(iter.carFeaturePic.size());
        content += split;
        content += iter.plates;
        content += split;
        content += iter.plateColor;
        content += "\n";

    }
    if (inDataFile.is_open()) {
//            printf("out:%s\n", content.c_str());
        inDataFile.write((const char *) content.c_str(), content.size());
        inDataFile.flush();
        inDataFile.close();
    } else {
//          Error("打开文件失败:%s", inDataFileNameO.c_str());
    }
}


