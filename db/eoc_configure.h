//
// Created by lining on 2023/10/26.
//

#ifndef EOC_CONFIGURE_H
#define EOC_CONFIGURE_H
#include "dbInfo.h"
#include <vector>

namespace eoc_configure {

    int tableInit();


    class DBDataVersion {
    private:
        std::string db = "./eoc_configure.db";
    public:
        int id;
        std::string version;
        std::string time;

        int deleteFromDB();

        int insertToDB();

        int selectFromDB();

    };


//核心板基础配置 ./eoc_configure.db base_set表
    class DBBaseSet {
    private:
        std::string db = "./eoc_configure.db";
    public:
        int id;
        int Index;      //设备序号
        std::string City;    //城市
        int IsUploadToPlatform; //是否上传到云平台
        int Is4Gmodel;  //是否为4G模式
        int IsIllegalCapture;   //是否开启违法抓拍
        int IsPrintIntersectionName;//是否打印路口名称
        std::string FilesServicePath;//文件服务器地址
        int FilesServicePort;   //文件服务器端口
        std::string MainDNS;         //DNS主服务器
        std::string AlternateDNS;    //DNS备用服务器
        std::string PlatformTcpPath; //云平台TCP地址
        int PlatformTcpPort;    //云平台TCP端口
        std::string PlatformHttpPath;    //云平台HTTP地址
        int PlatformHttpPort;   //云平台HTTP端口
        std::string SignalMachinePath;   //信号机地址
        int SignalMachinePort;  //信号机端口
        int IsUseSignalMachine; //是否启用信号机接口
        std::string NtpServerPath;   //NTP服务器地址
        std::string FusionMainboardIp;   //主控机Ip
        int FusionMainboardPort;    //主控机端口号
        std::string IllegalPlatformAddress;  //违停服务器地址
        std::string Remarks;     //备注

        int deleteFromDB();

        int insertToDB();

        int selectFromDB();
    };

//所属路口信息 ./eoc_configure.db  belong_intersection表
    class DBIntersection {
    private:
        std::string db = "./eoc_configure.db";
    public:
        int id;
        std::string Guid;    //guid
        std::string Name;    //路口名称
        int Type;   //路口的类型：1=十字形，2=X形，3=T形，4=Y形
        std::string PlatId;  //平台编号
        float XLength;  //x宽度(米)
        float YLength;  //y宽度(米)
        int LaneNumber; //车道数量
        std::string Latitude;    //路口中心点（原点）GPS坐标纬度
        std::string Longitude;   //路口中心点（原点）GPS坐标经度
        //-----路口参数设置
        int FlagEast;   // 十字路口设定标志位 是否启用偏移方向-东
        int FlagSouth;  // 十字路口设定标志位 是否启用偏移方向-南
        int FlagWest;   // 十字路口设定标志位  是否启用偏移方向-西
        int FlagNorth;  // 十字路口设定标志位  是否启用偏移方向-北
        double DeltaXEast;  // 偏移量X-东
        double DeltaYEast;  // 偏移量Y-东
        double DeltaXSouth; // 偏移量X-南
        double DeltaYSouth; // 偏移量Y-南
        double DeltaXWest;  // 偏移量X-西
        double DeltaYWest;  // 偏移量Y-西
        double DeltaXNorth; // 偏移量X-北
        double DeltaYNorth; // 偏移量Y-北
        double WidthX;      // 融合前去重重合车辆用偏移量X
        double WidthY;      // 融合前去重重合车辆用偏移量Y

        int deleteFromDB();

        int insertToDB();

        int selectFromDB();
    };

//融合参数设置 ./eoc_configure.db fusion_para_set表
    class DBFusionParaSet {
    private:
        std::string db = "./eoc_configure.db";
    public:
        int id;
        double IntersectionAreaPoint1X; //路口中心区域标点1-X
        double IntersectionAreaPoint1Y; //路口中心区域标点1-Y
        double IntersectionAreaPoint2X; //路口中心区域标点2-X
        double IntersectionAreaPoint2Y; //路口中心区域标点2-Y
        double IntersectionAreaPoint3X; //路口中心区域标点3-X
        double IntersectionAreaPoint3Y; //路口中心区域标点3-Y
        double IntersectionAreaPoint4X; //路口中心区域标点4-X
        double IntersectionAreaPoint4Y; //路口中心区域标点4-Y

        int deleteFromDB();

        int insertToDB();

        int selectFromDB();
    };

//关联设备 ./eoc_configure.db associated_equip表
    class DBAssociatedEquip {
    private:
        std::string db = "./eoc_configure.db";
    public:
        int id;
        int EquipType;  //设备类型
        std::string EquipCode;   //设备编码

        int deleteAllFromDB();

        int insertToDB();
    };

    int getAssociatedEquips(std::vector<DBAssociatedEquip> &data);

}

#endif //EOC_CONFIGURE_H
