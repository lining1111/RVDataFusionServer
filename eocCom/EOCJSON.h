//
// Created by lining on 2023/2/23.
//

#ifndef EOCJSON_H
#define EOCJSON_H

#include <xpack/json.h>

using namespace xpack;

#include <string>
#include <sys/time.h>
#include <glog/logging.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "db/DBTable.h"
#include "../version.h"

/*
 * 函数功能：产生uuid
 * 参数：无
 * 返回值：uuid的string
 * */
static std::string random_uuid() {
    char buf[37] = {0};
    struct timeval tmp;
    const char *c = "89ab";
    char *p = buf;
    unsigned int n, b;
    gettimeofday(&tmp, nullptr);
    srand(tmp.tv_usec);

    for (n = 0; n < 16; ++n) {
        b = rand() % 65536;
        switch (n) {
            case 6:
                sprintf(p, "4%x", b % 15);
                break;
            case 8:
                sprintf(p, "%c%x", c[rand() % strlen(c)], b % 15);
                break;
            default:
                sprintf(p, "%02x", b);
                break;
        }
        p += 2;
        switch (n) {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    *p = 0;
    return std::string(buf);
}

/*取板卡本地ip地址和n2n地址 */
static int getipaddr(char *ethip, char *n2nip) {
    int ret = 0;
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *pifAddrStruct = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    if (ifAddrStruct != NULL) {
        pifAddrStruct = ifAddrStruct;
    }

    while (ifAddrStruct != NULL) {
        if (ifAddrStruct->ifa_addr == NULL) {
            ifAddrStruct = ifAddrStruct->ifa_next;
            LOG(ERROR) << "addr null";
            continue;
        }

        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0) {
                LOG(INFO) << "eth0 IP Address:" << addressBuffer;
                sprintf(ethip, "%s", addressBuffer);
            } else if (strcmp(ifAddrStruct->ifa_name, "n2n0") == 0) {
                LOG(INFO) << "n2n0 IP Address:" << addressBuffer;
                sprintf(n2nip, "%s", addressBuffer);
            }
        }

        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    if (pifAddrStruct != NULL) {
        freeifaddrs(pifAddrStruct);
    }

    return ret;
}


class ReqHead {
public:
    std::string Version;
    std::string Code;
    std::string Guid;

XPACK(O(Version, Code, Guid));
};

class RspHead {
public:
    std::string Version;
    std::string Code;
    int State;
    std::string ResultMessage;
    std::string Guid;

XPACK(O(Version, Code, State, ResultMessage, Guid));
};

//心跳
class DataS100 {
    std::string Code = "MCCS100";
public:
XPACK(O(Code));
};

class S100 {
public:
    std::string Version;
    std::string Code;
public:
    std::string Guid;
    DataS100 Data;

    int get(std::string comVersion) {
        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = random_uuid();

        return 0;
    };

XPACK(O(Version, Code, Guid, Data));

private:
    std::string _code = "MCCS100";
};

class DataR100 {
    std::string Code = "MCCR100";

XPACK(O(Code));
};

class R100 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataS100 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion) {
        //TODO
        return 0;
    };

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

XPACK(O(Code, EquipNumber, EquipIp, EquipType, SoftVersion, DataVersion));
};

class S101 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS101 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion) {
        int ret = 0;
        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = random_uuid();

        //获取设备sn
        ret = dbGetUname(this->Data.EquipNumber);
        if (ret != 0) {
            LOG(ERROR) << "get uname err";
            return -1;
        }
        //获取ip信息 ip[n2n]
        char ethip[32] = {0};
        char n2nip[32] = {0};
        char ipmsg[128] = {0};
        memset(ethip, 0, 32);
        memset(n2nip, 0, 32);
        memset(ipmsg, 0, 128);
        getipaddr(ethip, n2nip);
        sprintf(ipmsg, "%s[%s]", ethip, n2nip);
        this->Data.EquipIp = std::string(ipmsg);
        //获取dataVersion
        DBDataVersion dbDataVersion;
        dbDataVersion.selectFromDB();
        this->Data.DataVersion = dbDataVersion.version;

        this->Data.EquipType = "XX";
        this->Data.SoftVersion = VERSION_BUILD_TIME;

        return 0;
    };

private:
    std::string _code = "MCCS101";;
};

class DataR101 {
public:
    std::string Code = "MCCR101";
    int State;
    std::string Message;

XPACK(O(Code, State, Message));

};

class R101 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR101 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion) {
        //TODO
        return 0;
    };

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

XPACK(O(FlagEast, FlagSouth, FlagWest, FlagNorth,
        DeltaXEast, DeltaYEast, DeltaXSouth, DeltaYSouth, DeltaXWest, DeltaYWest, DeltaXNorth, DeltaYNorth,
        WidthX, WidthY));

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

XPACK(O(Guid, Name, Type, PlatId, XLength, YLength, LaneNumber,
        Latitude, Longitude, IntersectionBaseSetting));
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

XPACK(O(City, IsUploadToPlatform, Is4Gmodel, IsIllegalCapture,
        IsPrintIntersectionName, Remarks, FilesServicePath, FilesServicePort,
        MainDNS, AlternateDNS, PlatformTcpPath, PlatformTcpPort, PlatformHttpPath,
        PlatformHttpPort, SignalMachinePath, SignalMachinePort, IsUseSignalMachine,
        NtpServerPath, IllegalPlatformAddress, FusionMainboardIp, FusionMainboardPort));

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

XPACK(O(IntersectionAreaPoint1X, IntersectionAreaPoint1Y,
        IntersectionAreaPoint2X, IntersectionAreaPoint2Y,
        IntersectionAreaPoint3X, IntersectionAreaPoint3Y,
        IntersectionAreaPoint4X, IntersectionAreaPoint4Y));
};

class AssociatedEquip {
public:
    int EquipType;
    std::string EquipCode;

XPACK(O(EquipType, EquipCode));
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

XPACK(O(DataVersion, IntersectionInfo, Index, BaseSetting, FusionParaSetting, AssociatedEquips));
};

class R102 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR102 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion) {
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR102";
};

class DataS102 {
public:
    std::string Code = "MCCS102";
    int State;
    std::string Messsage;

XPACK(O(Code, State, Messsage));
};

class S102 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS102 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion, std::string guuid, int state, std::string message) {
        this->Data.State = state;
        this->Data.Messsage = message;
        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = guuid;
    };

private:
    std::string _code = "MCCS102";
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

XPACK(O(Code, DataVersion, IntersectionInfo, Index,
        BaseSetting, FusionParaSetting, AssociatedEquips));
};

class R104 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR102 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion) {
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR104";
};

class DataS104 {
public:
    std::string Code = "MCCS104";
    std::string MainboardGuid;

XPACK(O(Code, MainboardGuid));
};

class S104 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS104 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion) {
        int ret = 0;
        //获取设备sn
        ret = dbGetUname(this->Data.MainboardGuid);
        if (ret != 0) {
            LOG(ERROR) << "get uname err";
            return -1;
        }

        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = random_uuid();
        return 0;
    };

private:
    std::string _code = "MCCS104";
};

//外围状态上传接口
class DataS105 {
public:
    std::string Code = "MCCS105";
    int Total;
    int Success;

XPACK(O(Code, Total, Success));
};

class S105 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS105 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion, int total, int success) {
        this->Data.Total = total;
        this->Data.Success = success;

        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = random_uuid();
        return 0;
    };

private:
    std::string _code = "MCCS105";
};

class DataR105 {
public:
    std::string Code = "MCCR105";
    int State;
    std::string Message;

XPACK(O(Code, State, Message));
};

class R105 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR105 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion){
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR105";
};

class DataR106 {
public:
    std::string Code = "MCCR106";
    std::string FileName;
    std::string FileSize;
    std::string FileVersion;
    std::string DownloadPath;
    std::string FileMD5;

XPACK(O(Code, FileName, FileSize, FileVersion, DownloadPath, FileMD5));
};

class R106 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR106 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion){
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR106";
};

class DataS106 {
public:
    std::string Code = "MCCS106";
    int ResultType;
    int State;
    int Progress;
    std::string Message;

XPACK(O(Code, ResultType, State, Progress, Message));
};

class S106 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS106 Data;

XPACK(O(Version, Code, Guid, Data));

    /**
    * 发送下载进度
    * @param data out
    * @param resultType 1:开始 2：完成
    * @param state 1：成功 0：失败
    * @param progress 0-100
    * @param message 进度说明
    * @return 0:成功
    */
    int get(std::string comVersion, std::string guuid, int resultType, int state, int progress, std::string message){
        this->Data.ResultType = resultType;
        this->Data.State = state;
        this->Data.Progress = progress;
        this->Data.Message = message;

        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = guuid;
        return 0;
    };

private:
    std::string _code = "MCCS106";
};

//主控机软件升级确认接口
class DataR107 {
public:
    std::string Code = "MCCR107";
    std::string FileName;
    std::string FileVersion;
    int UpgradeMode;
    std::string FileMD5;
    std::string CurrentSoftVersion;
    std::string HardwareVersion;

XPACK(O(Code, FileName, FileVersion, UpgradeMode, FileMD5, CurrentSoftVersion, HardwareVersion));
};

class R107 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR107 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion){
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR107";
};

class DataS107 {
public:
    std::string Code = "MCCS107";
    int ResultType;
    int State;
    int Progress;
    std::string Message;

XPACK(O(Code, ResultType, State, Progress, Message));
};

class S107 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS107 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion, std::string guuid, int result, int state, int progress, std::string message){
        this->Data.ResultType = result;
        this->Data.State = state;
        this->Data.Progress = progress;
        this->Data.Message = message;

        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = guuid;
    };

private:
    std::string _code = "MCCS107";
};

//服务器地址推送接口
class DataR108 {
public:
    std::string Code = "MCCR108";

XPACK(O(Code));
};

class R108 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR108 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion){
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR108";
};

class DataS108 {
public:
    std::string Code = "MCCS108";

XPACK(O(Code));
};

class S108 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS108 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion){
        this->Version = comVersion;
        this->Code = this->_code;
        this->Guid = random_uuid();
        return 0;
    };

private:
    std::string _code = "MCCS108";
};

#endif //EOCJSON_H
