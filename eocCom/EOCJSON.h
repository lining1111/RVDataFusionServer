//
// Created by lining on 2023/2/23.
//

#ifndef EOCJSON_H
#define EOCJSON_H

#include <json/json.h>
#include <string>

class ReqHead {
    std::string Version;
    std::string Code;
public:
    std::string Guid;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string version, std::string code);
};

class RspHead {

    std::string Version;
    std::string Code;
    int State;
    std::string ResultMessage;
public:
    std::string Guid;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string version, std::string code, int state, std::string resultMessage);
};

//心跳
class DataS100 {
    std::string Code = "MCCS100";
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S100 {
public:
    ReqHead head;
    DataS100 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCS100";
};

class DataR100 {
    std::string Code = "MCCR100";

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R100 {
public:
    RspHead head;
    DataS100 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCR100";
};


//登录
class DataS101 {
    std::string Code = "MCCS101";
public:
    std::string EquipNumber;
    std::string EquipIp;
    std::string EquipType;
    std::string SoftVersion;
    std::string DataVersion;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S101 {
public:
    ReqHead head;
    DataS101 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCS101";;
};

class DataR101 {
    std::string Code = "MCCR101";
public:
    int State;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};

class R101 {
public:
    RspHead head;
    DataR101 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCR101";
};

//配置下发接口
class IntersectionBaseSettingEntity {
public:
    int FlagEast;
    int FlagSouth;
    int FlagWest;
    int FlagNorth;
    double DeltaXEast;
    double DeltaYEast;
    double DeltaXSouth;
    double DeltaYSouth;
    double DeltaXWest;
    double DeltaYWest;
    double DeltaXNorth;
    double DeltaYNorth;
    double WidthX;
    double WidthY;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};


class IntersectionEntity {
public:
    std::string Guid;
    std::string Name;
    int Type;
    std::string PlatId;
    float XLength;
    float YLength;
    int LaneNumber;
    std::string Latitude;
    std::string Longitude;
    IntersectionBaseSettingEntity IntersectionBaseSetting;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class BaseSettingEntity {
public:
    std::string City;
    int IsUploadToPlatform;
    int Is4Gmodel;
    int IsIllegalCapture;
    int IsPrintIntersectionName;
    std::string Remarks;
    std::string FilesServicePath;
    int FilesServicePort;
    std::string MainDNS;
    std::string AlternateDNS;
    std::string PlatformTcpPath;
    int PlatformTcpPort;
    std::string PlatformHttpPath;
    int PlatformHttpPort;
    std::string SignalMachinePath;
    int SignalMachinePort;
    int IsUseSignalMachine;
    std::string NtpServerPath;
    std::string IllegalPlatformAddress;
    std::string FusionMainboardIp;
    int FusionMainboardPort;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};

class FusionParaEntity {
public:
    double IntersectionAreaPoint1X;
    double IntersectionAreaPoint1Y;
    double IntersectionAreaPoint2X;
    double IntersectionAreaPoint2Y;
    double IntersectionAreaPoint3X;
    double IntersectionAreaPoint3Y;
    double IntersectionAreaPoint4X;
    double IntersectionAreaPoint4Y;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class AssociatedEquip {
public:
    int EquipType;
    std::string EquipCode;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class DataR102 {
    std::string Code = "MCCR102";
public:
    std::string DataVersion;
    IntersectionEntity IntersectionInfo;
    int Index;
    BaseSettingEntity BaseSetting;
    FusionParaEntity FusionParaSetting;
    std::vector<AssociatedEquip> AssociatedEquips;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R102 {
public:
    RspHead head;
    DataR102 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCR102";
};

class DataS102 {
    std::string Code = "MCCS102";
public:
    int State;
    std::string Messsage;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S102 {
public:
    ReqHead head;
    DataS102 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion, std::string guuid, int state, std::string message);

private:
    std::string _code="MCCS102";
};

//设备主动获取配置
class DataR104 {
    std::string Code = "MCCR104";
public:
    std::string DataVersion;
    IntersectionEntity IntersectionInfo;
    int Index;
    BaseSettingEntity BaseSetting;
    FusionParaEntity FusionParaSetting;
    std::vector<AssociatedEquip> AssociatedEquips;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R104 {
public:
    RspHead head;
    DataR102 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code="MCCR104";
};

class DataS104 {
    std::string Code = "MCCS104";
public:
    std::string MainboardGuid;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S104 {
public:
    ReqHead head;
    DataS104 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion);

private:
    std::string _code = "MCCS104";
};

//外围状态上传接口
class DataS105 {
    std::string Code = "MCCS105";
public:
    int Total;
    int Success;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S105 {
public:
    ReqHead head;
    DataS105 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion, int total, int success);

private:
    std::string _code="MCCS105";
};

class DataR105 {
    std::string Code = "MCCR105";
public:
    int State;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R105 {
public:
    RspHead head;
    DataR105 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
    int get(std::string comVersion);

private:
    std::string _code = "MCCR105";
};

class DataR106 {
    std::string Code = "MCCR106";
public:
    std::string FileName;
    std::string FileSize;
    std::string FileVersion;
    std::string DownloadPath;
    std::string FileMD5;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R106 {
public:
    RspHead head;
    DataR106 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
    int get(std::string comVersion);

private:
    std::string _code="MCCR106";
};

class DataS106 {
    std::string Code = "MCCS106";
public:
    int ResultType;
    int State;
    int Progress;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S106 {
public:
    ReqHead head;
    DataS106 Data;
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
 /**
 * 发送下载进度
 * @param data out
 * @param resultType 1:开始 2：完成
 * @param state 1：成功 0：失败
 * @param progress 0-100
 * @param message 进度说明
 * @return 0:成功
 */
 int get(std::string comVersion, std::string guuid, int resultType, int state, int progress, std::string message);

private:
    std::string _code="MCCS106";
};

//主控机软件升级确认接口
class DataR107 {
    std::string Code = "MCCR107";
public:
    std::string FileName;
    std::string FileVersion;
    int UpgradeMode;
    std::string FileMD5;
    std::string CurrentSoftVersion;
    std::string HardwareVersion;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};

class R107 {
public:
    RspHead head;
    DataR107 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
    int get(std::string comVersion);

private:
    std::string _code="MCCR107";
};

class DataS107 {
    std::string Code = "MCCS107";
public:
    int ResultType;
    int State;
    int Progress;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S107 {
public:
    ReqHead head;
    DataS107 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

    int get(std::string comVersion, std::string guuid, int result, int state, int progress, std::string message);

private:
    std::string _code="MCCS107";
};

//服务器地址推送接口
class DataR108 {
    std::string Code = "MCCR108";
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class R108 {
public:
    RspHead head;
    DataR108 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
    int get(std::string comVersion);

private:
    std::string _code="MCCR108";
};

class DataS108 {
    std::string Code = "MCCS108";
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class S108 {
public:
    ReqHead head;
    DataS108 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
    int get(std::string comVersion);

private:
    std::string _code="MCCS108";
};

#endif //EOCJSON_H
