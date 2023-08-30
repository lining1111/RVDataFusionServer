//
// Created by lining on 6/30/22.
//

#include <valarray>
#include "mergeStruct.h"

const int INF = 0x7FFFFFFF;

void OBJECT_INFO_T2ObjTarget(OBJECT_INFO_T &objectInfoT, ObjTarget &objTarget) {
    objTarget.objID = to_string(objectInfoT.objID);
    objTarget.objCameraID = objectInfoT.cameraID;
    objTarget.objRadarID = objectInfoT.radarID;
    objTarget.objType = objectInfoT.objType;
    objTarget.objSourceType = objectInfoT.objSourceType;

    objTarget.left = objectInfoT.left;
    objTarget.top = objectInfoT.top;
    objTarget.right = objectInfoT.right;


    objTarget.plates = objectInfoT.plates;
    objTarget.plateColor = objectInfoT.plateColor;
    objTarget.carFeaturePic = objectInfoT.carFeaturePic;
    objTarget.laneCode = objectInfoT.laneCode;
    objTarget.carType = objectInfoT.carType;
    objTarget.carLength = objectInfoT.carLength;

    objTarget.bottom = objectInfoT.bottom;
    objTarget.locationX = objectInfoT.locationX;
    objTarget.locationY = objectInfoT.locationY;
    objTarget.directionAngle = objectInfoT.directionAngle;
    objTarget.speedX = objectInfoT.speedX;
    objTarget.speedY = objectInfoT.speedY;
    objTarget.longitude = objectInfoT.longitude;
    objTarget.latitude = objectInfoT.latitude;
}

void ObjTarget2OBJECT_INFO_T(ObjTarget &objTarget, OBJECT_INFO_T &objectInfoT) {

    objectInfoT.objID = atoi(objTarget.objID.c_str());
    objectInfoT.inputID = objTarget.objID;

    objectInfoT.cameraID = objTarget.objCameraID;
    objectInfoT.radarID = objTarget.objRadarID;

    objectInfoT.objType = objTarget.objType;
    objectInfoT.objSourceType = objTarget.objSourceType;

    objectInfoT.left = objTarget.left;
    objectInfoT.top = objTarget.top;
    objectInfoT.right = objTarget.right;
    objectInfoT.bottom = objTarget.bottom;
    objectInfoT.locationX = objTarget.locationX;
    objectInfoT.locationY = objTarget.locationY;

    objectInfoT.plates = objTarget.plates;
    objectInfoT.plateColor = objTarget.plateColor;
    objectInfoT.carFeaturePic = objTarget.carFeaturePic;
    objectInfoT.laneCode = objTarget.laneCode;
    objectInfoT.carType = objTarget.carType;
    objectInfoT.carLength = objTarget.carLength;

    objectInfoT.directionAngle = objTarget.directionAngle;

    objectInfoT.speedX = objTarget.speedX;
    objectInfoT.speedY = objTarget.speedY;

    objectInfoT.longitude = objTarget.longitude;
    objectInfoT.latitude = objTarget.latitude;

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

    objectInfoNew.left = objectInfoT.left;
    objectInfoNew.top = objectInfoT.top;
    objectInfoNew.right = objectInfoT.right;
    objectInfoNew.bottom = objectInfoT.bottom;
    objectInfoNew.locationX = objectInfoT.locationX;
    objectInfoNew.locationY = objectInfoT.locationY;

    objectInfoNew.plates = objectInfoT.plates;
    objectInfoNew.plateColor = objectInfoT.plateColor;
    objectInfoNew.carFeaturePic = objectInfoT.carFeaturePic;
    objectInfoNew.laneCode = objectInfoT.laneCode;
    objectInfoNew.carType = objectInfoT.carType;
    objectInfoNew.carLength = objectInfoT.carLength;
    objectInfoNew.directionAngle = objectInfoT.directionAngle;
    objectInfoNew.speed = sqrt(objectInfoT.speedX * objectInfoT.speedX + objectInfoT.speedY * objectInfoT.speedY);
    objectInfoNew.longitude = objectInfoT.longitude;
    objectInfoNew.latitude = objectInfoT.latitude;
}

void OBJECT_INFO_NEW2ObjMix(OBJECT_INFO_NEW &objectInfoNew, ObjMix &objMix) {
    objMix.objID = to_string(objectInfoNew.showID);
    objMix.objType = objectInfoNew.objType;
    objMix.angle = objectInfoNew.directionAngle;
    objMix.speed = objectInfoNew.speed;
    objMix.locationX = objectInfoNew.locationX;
    objMix.locationY = objectInfoNew.locationY;
    objMix.longitude = objectInfoNew.longitude;
    objMix.latitude = objectInfoNew.latitude;
    objMix.flagNew = objectInfoNew.flag_new;

    //添加属性
    objMix.laneCode = objectInfoNew.laneCode;
    objMix.carLength = objectInfoNew.carLength;
    objMix.carFeaturePic = objectInfoNew.carFeaturePic;
    objMix.plates = objectInfoNew.plates;
    objMix.plateColor = objectInfoNew.plateColor;
    objMix.carType = objectInfoNew.carType;
}
