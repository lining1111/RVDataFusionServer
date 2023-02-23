//
// Created by lining on 2023/2/23.
//

#ifndef EOCJSON_H
#define EOCJSON_H

#include <json/json.h>
#include <string>

struct ReqHead {
    std::string Guid;
    std::string Version;
    std::string Code;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct RspHead {
    std::string Guid;
    std::string Version;
    std::string Code;
    int State;
    std::string ResultMessage;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//心跳
struct DataS100 {
    std::string Code = "MCCS100";

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S100 {
    ReqHead head;
    DataS100 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataR100 {
    std::string Code = "MCCR100";

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R100 {
    RspHead head;
    DataS100 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};


//登录
struct DataS101 {
    std::string Code = "MCCS101";
    std::string EquipNumber;
    std::string EquipIp;
    std::string EquipType;
    std::string SoftVersion;
    std::string DataVersion;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S101 {
    ReqHead head;
    DataS101 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataR101 {
    std::string Code = "MCCR101";
    int State;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};

struct R101 {
    RspHead head;
    DataR101 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//配置下发接口
struct IntersectionBaseSettingEntity {
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


struct IntersectionEntity {
    std::string Guid;
    std::string Name;
    int Type;
    std::string PlatId;
    float XLength;
    float YLength;
    int LaneNumber;
    std::string Latitude;
    std::string longitude;
    IntersectionBaseSettingEntity IntersectionBaseSetting;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct BaseSettingEntity {
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

struct FusionParaEntity {
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

struct AssociatedEquip {
    int EquipType;
    std::string EquipCode;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataR102 {
    std::string Code = "MCCR102";
    std::string DataVersion;
    IntersectionEntity IntersectionInfo;
    int Index;
    BaseSettingEntity BaseSetting;
    FusionParaEntity FusionParaSetting;
    std::vector<AssociatedEquip> AssociatedEquips;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R102 {
    RspHead head;
    DataR102 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataS102 {
    std::string Code = "MCCS102";
    int State;
    std::string Messsage;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S102 {
    ReqHead head;
    DataS102 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//设备主动获取配置
struct DataR104 {
    std::string Code = "MCCR104";
    std::string DataVersion;
    IntersectionEntity IntersectionInfo;
    int Index;
    BaseSettingEntity BaseSetting;
    FusionParaEntity FusionParaSetting;
    std::vector<AssociatedEquip> AssociatedEquips;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R104 {
    RspHead head;
    DataR102 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataS104 {
    std::string Code = "MCCS104";
    std::string MainboardGuid;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S104 {
    ReqHead head;
    DataS104 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//外围状态上传接口
struct DataS105 {
    std::string Code = "MCCS105";
    int Total;
    int Success;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S105 {
    ReqHead head;
    DataS105 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataR105 {
    std::string Code = "MCCR105";
    int State;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R105 {
    RspHead head;
    DataR105 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataR106 {
    std::string Code = "MCCR106";
    std::string FileName;
    std::string FileSize;
    std::string FileVersion;
    std::string DownloadPath;
    std::string FileMD5;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R106 {
    RspHead head;
    DataR106 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataS106 {
    std::string Code = "MCCS106";
    int ResultType;
    int State;
    int Progress;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S106 {
    ReqHead head;
    DataS106 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//主控机软件升级确认接口
struct DataR107 {
    std::string Code = "MCCR107";
    std::string FileName;
    std::string FileVersion;
    int UpgradeMode;
    std::string FileMD5;
    std::string CurrentSoftVersion;
    std::string HardwareVersion;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);

};

struct R107 {
    RspHead head;
    DataR107 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataS107 {
    std::string Code = "MCCS107";
    int ResultType;
    int State;
    int Progress;
    std::string Message;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S107 {
    ReqHead head;
    DataS107 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

//服务器地址推送接口
struct DataR108 {
    std::string Code = "MCCR108";

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct R108 {
    RspHead head;
    DataR108 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct DataS108 {
    std::string Code = "MCCS108";

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

struct S108 {
    RspHead head;
    DataR108 Data;

    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

#endif //EOCJSON_H
