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
#include <uuid/uuid.h>
#include <iostream>
#include <sys/statfs.h>
#include <net/if.h>
#include <sys/ioctl.h>

static int get_mac(char * mac_buff)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        close(sock);
        return -1;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    char szMac[64];
    int count = 0;
    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    count++;
                    unsigned char * ptr ;
                    ptr = (unsigned char  *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                    snprintf(szMac,64,"%02x:%02x:%02x:%02x:%02x:%02x",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
                    sprintf(mac_buff, "%s", szMac);
                    break;
                }
            }
        }else{
            close(sock);
            return -1;
        }
    }

    close(sock);
    return 0;
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
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);

        this->Guid = string(uuid_str);

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
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);
        this->Guid = string(uuid_str);

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
        if(strlen(n2nip)==0){
            get_mac(n2nip);
        }

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

//主控机状态
class PartsState_t {
public:
    int State = 1;
    int Size = 0;
    int ResidualSize = 0;
XPACK(O(State, Size, ResidualSize))
};

class MainBoardState_t {
public:
    string MainboardGuid;
    int State = 1;
    int CpuState = 1;
    double CpuUtilizationRatio = 0.0;
    double CpuTemperature = 0.0;
    int MemorySize = 0;
    int ResidualMemorySize = 0;
    string ModelVersion;
    string MainboardType;
    vector<PartsState_t> TFCardStates;
    vector<PartsState_t> EmmcStates;
    vector<PartsState_t> ExternalHardDisk;

XPACK(O(MainboardGuid, State, CpuState, CpuUtilizationRatio, CpuTemperature, MemorySize,
        ResidualMemorySize, ModelVersion, MainboardType, TFCardStates, EmmcStates, ExternalHardDisk));

    int get() {
        string equip_num;
        dbGetUname(equip_num);
        MainboardGuid = equip_num;

        State = 1;

        CpuState = 1;

        //TODO cpu利用率
        CpuUtilizationRatio = cpuUtilizationRatio();

        //cpu温度
        CpuTemperature = cpuTemperature();

        //内存大小
        MemorySize = memorySize();

        //剩余内存大小
        ResidualMemorySize = residualMemorySize();

        //模型版本
        ModelVersion = "NX";

        //主控版类型
        MainboardType = "NX";

        //TF卡状态
        {
            PartsState_t state;
            state.State = 1;
            state.Size = TF_Size();
            state.ResidualSize = TF_ResidualSize();
            if (state.Size == 0) {
                state.State = 0;
            }

            TFCardStates.push_back(state);
        }

        //emmc状态
        {
            PartsState_t state;
            state.State = 1;
            state.Size = EMMC_Size();
            state.ResidualSize = EMMC_ResidualSize();
            if (state.Size == 0) {
                state.State = 0;
            }
            EmmcStates.push_back(state);
        }

        //外接硬盘状态
        {
            PartsState_t state;
            state.State = 1;
            state.Size = ExternalHardDisk_Size();
            state.ResidualSize = ExternalHardDisk_ResidualSize();
            if (state.Size == 0) {
                state.State = 0;
            }
            ExternalHardDisk.push_back(state);
        }

        return 0;
    }


private:
    std::string getValueBySystemCommand(std::string strCommand) {
        //printf("-----%s------\n",strCommand.c_str());
        std::string strData = "";
        FILE *fp = NULL;
        char buffer[128];
        fp = popen(strCommand.c_str(), "r");
        if (fp) {
            while (fgets(buffer, sizeof(buffer), fp)) {
                strData.append(buffer);
            }
            pclose(fp);
        }
        //printf("-----%s------ end\n",strCommand.c_str());
        return strData;
    };

    string trim(string str) {
        if (str.empty()) {
            return str;
        }
        str.erase(0, str.find_first_not_of(" "));
        str.erase(str.find_last_not_of(" ") + 1);
        return str;
    };

    std::vector<std::string> split(std::string str, std::string pattern) {
        string::size_type pos;
        vector<string> result;
        str += pattern;
        int size = str.size();

        for (int i = 0; i < size; i++) {
            pos = str.find(pattern, i);
            if (pos < size) {
                string s = str.substr(i, pos - i);
                result.push_back(s);
                i = pos + pattern.size() - 1;
            }
        }

        return result;
    };


    double cpuUtilizationRatio() {

        std::string strRes = getValueBySystemCommand("top -b -n 1 |grep Cpu | cut -d \",\" -f 1 | cut -d \":\" -f 2");

        strRes = trim(strRes);
        printf("--%s--  strRes.size : %lu \n", strRes.c_str(), strRes.length());
        std::vector<std::string> vecT = split(strRes, " ");
        if (vecT.size() == 2) {
            printf("%s\n", vecT.at(0).c_str());
            return atof(vecT.at(0).c_str());
        }
        return 0;
    };


    double cpuTemperature() {
        FILE *fp = NULL;
        int temp = 0;
        fp = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
        if (fp == nullptr) {
            std::cerr << "CpuTemperature fail" << std::endl;
            return 0;
        }

        fscanf(fp, "%d", &temp);
        fclose(fp);
        return (double) temp / 1000;
    };

    double memorySize() {
        int mem_free = -1;//空闲的内存，=总内存-使用了的内存
        int mem_total = -1; //当前系统可用总内存
        int mem_buffers = -1;//缓存区的内存大小
        int mem_cached = -1;//缓存区的内存大小
        char name[20];

        FILE *fp;
        char buf1[128], buf2[128], buf3[128], buf4[128], buf5[128];
        int buff_len = 128;
        fp = fopen("/proc/meminfo", "r");
        if (fp == NULL) {
            std::cerr << "GetSysMemInfo() error! file not exist" << std::endl;
            return -1;
        }
        if (NULL == fgets(buf1, buff_len, fp) ||
            NULL == fgets(buf2, buff_len, fp) ||
            NULL == fgets(buf3, buff_len, fp) ||
            NULL == fgets(buf4, buff_len, fp) ||
            NULL == fgets(buf5, buff_len, fp)) {
            std::cerr << "GetSysMemInfo() error! fail to read!" << std::endl;
            fclose(fp);
            return -1;
        }
        fclose(fp);
        sscanf(buf1, "%s%d", name, &mem_total);
        sscanf(buf2, "%s%d", name, &mem_free);
        sscanf(buf4, "%s%d", name, &mem_buffers);
        sscanf(buf5, "%s%d", name, &mem_cached);
        return (double) mem_total / 1024.0;
    };

    double residualMemorySize() {
        int mem_free = -1;//空闲的内存，=总内存-使用了的内存
        int mem_total = -1; //当前系统可用总内存
        int mem_buffers = -1;//缓存区的内存大小
        int mem_cached = -1;//缓存区的内存大小
        char name[20];

        FILE *fp;
        char buf1[128], buf2[128], buf3[128], buf4[128], buf5[128];
        int buff_len = 128;
        fp = fopen("/proc/meminfo", "r");
        if (fp == NULL) {
            std::cerr << "GetSysMemInfo() error! file not exist" << std::endl;
            return -1;
        }
        if (NULL == fgets(buf1, buff_len, fp) ||
            NULL == fgets(buf2, buff_len, fp) ||
            NULL == fgets(buf3, buff_len, fp) ||
            NULL == fgets(buf4, buff_len, fp) ||
            NULL == fgets(buf5, buff_len, fp)) {
            std::cerr << "GetSysMemInfo() error! fail to read!" << std::endl;
            fclose(fp);
            return -1;
        }
        fclose(fp);
        sscanf(buf1, "%s%d", name, &mem_total);
        sscanf(buf2, "%s%d", name, &mem_free);
        sscanf(buf4, "%s%d", name, &mem_buffers);
        sscanf(buf5, "%s%d", name, &mem_cached);
        return (double) mem_free / 1024.0;
    };

    static double TF_Size() {
        string devStr = "~/mnt_tf";
        struct statfs diskInfo;
        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) totalsize / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

    static double TF_ResidualSize() {
        string devStr = "~/mnt_tf";
        struct statfs diskInfo;

        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) freeDisk / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

    static double EMMC_Size() {
        string devStr = "/";
        struct statfs diskInfo;

        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) totalsize / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

    static double EMMC_ResidualSize() {
        string devStr = "/";
        struct statfs diskInfo;

        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) freeDisk / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

    static double ExternalHardDisk_Size() {
        string devStr = "/mnt/mnt_hd";
        struct statfs diskInfo;

        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) totalsize / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

    static double ExternalHardDisk_ResidualSize() {
        string devStr = "/mnt/mnt_hd";
        struct statfs diskInfo;

        // 设备挂载的节点
        if (statfs(devStr.c_str(), &diskInfo) == 0) {
            uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
            uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
            uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
            uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
            return (double) freeDisk / (1024.0 * 1024.0);
        } else {
            return 0;
        }
    }

};

class DataS103 {
public:
    std::string Code = "MCCS103";
    MainBoardState_t MainBoardState;

XPACK(O(Code, MainBoardState));
};

class S103 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    DataS103 Data;

XPACK(O(Version, Code, Guid, Data));

    int get(std::string comVersion) {
        this->Version = comVersion;
        this->Code = this->_code;
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);
        this->Guid = string(uuid_str);

        this->Data.MainBoardState.get();

        return 0;
    };

private:
    std::string _code = "MCCS103";
};

class DataR103 {
public:
    std::string Code = "MCCR103";
    int State;
    std::string Message;

XPACK(O(Code, State, Message));
};

class R103 {
public:
    std::string Version;
    std::string Code;
    std::string Guid;
    int State;
    std::string ResultMessage;
    DataR103 Data;

XPACK(O(Version, Code, Guid, State, ResultMessage, Data));

    int get(std::string comVersion) {
        //TODO
        return 0;
    };

private:
    std::string _code = "MCCR103";
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
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);
        this->Guid = string(uuid_str);
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
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);
        this->Guid = string(uuid_str);
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

    int get(std::string comVersion) {
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

    int get(std::string comVersion) {
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
    int get(std::string comVersion, std::string guuid, int resultType, int state, int progress, std::string message) {
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

    int get(std::string comVersion) {
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

    int get(std::string comVersion, std::string guuid, int result, int state, int progress, std::string message) {
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

    int get(std::string comVersion) {
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

    int get(std::string comVersion) {
        this->Version = comVersion;
        this->Code = this->_code;
        uuid_t uuid;
        char uuid_str[37];
        memset(uuid_str, 0, 37);
        uuid_generate_time(uuid);
        uuid_unparse(uuid, uuid_str);
        this->Guid = string(uuid_str);
        return 0;
    };

private:
    std::string _code = "MCCS108";
};

#endif //EOCJSON_H
