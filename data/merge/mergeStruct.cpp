//
// Created by lining on 6/30/22.
//

#include <valarray>
#include <string>
#include "mergeStruct.h"

const int INF = 0x7FFFFFFF;

void OBJECT_INFO_T2ObjTarget(OBJECT_INFO_T &objectInfoT, ObjTarget &objTarget) {
    objTarget.objID = objectInfoT.objID;
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

    objectInfoT.objID = objTarget.objID;

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

}

void OBJECT_INFO_T2OBJECT_INFO_NEW(OBJECT_INFO_T &objectInfoT, OBJECT_INFO_NEW &objectInfoNew) {
    objectInfoNew.objID.push_back(objectInfoT.objID);
    objectInfoNew.objID.push_back(-INF);
    objectInfoNew.objID.push_back(-INF);
    objectInfoNew.objID.push_back(-INF);
    objectInfoNew.cameraID.push_back(objectInfoT.cameraID);
    objectInfoNew.cameraID.push_back(-INF);
    objectInfoNew.cameraID.push_back(-INF);
    objectInfoNew.cameraID.push_back(-INF);
    objectInfoNew.cameraID.push_back(-INF);
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
    objectInfoNew.speedX = objectInfoT.speedX;
    objectInfoNew.speedY = objectInfoT.speedY;
    objectInfoNew.longitude = objectInfoT.longitude;
    objectInfoNew.latitude = objectInfoT.latitude;
}
