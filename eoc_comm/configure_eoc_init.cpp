
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "sqlite_eoc_db.h"
#include "logger.h"
#include "configure_eoc_init.h"


EOC_Base_Set g_eoc_base_set;    //基础配置
EOC_Intersection g_eoc_intersection;    //路口信息
EOC_Fusion_Para_Set g_eoc_fusion_para;  //融合参数
vector<EOC_Associated_Equip> g_eoc_associated_equip;  //主控机关联设备

int g_eoc_base_set_init(void)
{
    int ret = 0;
    DB_Base_Set_T data;

    string db_data_version = "";
    db_version_get(db_data_version);
    snprintf(g_eoc_base_set.EocConfDataVersion, sizeof(g_eoc_base_set.EocConfDataVersion), "%s", db_data_version.c_str());

    ret = db_base_set_get(data);
    if(0 == ret)
    {
        g_eoc_base_set.Index = data.Index;
        snprintf(g_eoc_base_set.City, sizeof(g_eoc_base_set.City), "%s", data.City.c_str());
        g_eoc_base_set.IsUploadToPlatform = data.IsUploadToPlatform;
        g_eoc_base_set.Is4Gmodel = data.Is4Gmodel;
        g_eoc_base_set.IsIllegalCapture = data.IsIllegalCapture;
        snprintf(g_eoc_base_set.Remarks, sizeof(g_eoc_base_set.Remarks), "%s", data.Remarks.c_str());
        g_eoc_base_set.IsPrintIntersectionName = data.IsPrintIntersectionName;
        snprintf(g_eoc_base_set.FilesServicePath, sizeof(g_eoc_base_set.FilesServicePath), "%s", data.FilesServicePath.c_str());
        g_eoc_base_set.FilesServicePort = data.FilesServicePort;
        snprintf(g_eoc_base_set.MainDNS, sizeof(g_eoc_base_set.MainDNS), "%s", data.MainDNS.c_str());
        snprintf(g_eoc_base_set.AlternateDNS, sizeof(g_eoc_base_set.AlternateDNS), "%s", data.AlternateDNS.c_str());
        snprintf(g_eoc_base_set.PlatformTcpPath, sizeof(g_eoc_base_set.PlatformTcpPath), "%s", data.PlatformTcpPath.c_str());
        g_eoc_base_set.PlatformTcpPort = data.PlatformTcpPort;
        snprintf(g_eoc_base_set.PlatformHttpPath, sizeof(g_eoc_base_set.PlatformHttpPath), "%s", data.PlatformHttpPath.c_str());
        g_eoc_base_set.PlatformHttpPort = data.PlatformHttpPort;
        snprintf(g_eoc_base_set.SignalMachinePath, sizeof(g_eoc_base_set.SignalMachinePath), "%s", data.SignalMachinePath.c_str());
        g_eoc_base_set.SignalMachinePort = data.SignalMachinePort;
        g_eoc_base_set.IsUseSignalMachine = data.IsUseSignalMachine;
        snprintf(g_eoc_base_set.NtpServerPath, sizeof(g_eoc_base_set.NtpServerPath), "%s", data.NtpServerPath.c_str());
        snprintf(g_eoc_base_set.FusionMainboardIp, sizeof(g_eoc_base_set.FusionMainboardIp), "%s", data.FusionMainboardIp.c_str());
        g_eoc_base_set.FusionMainboardPort = data.FusionMainboardPort;
        snprintf(g_eoc_base_set.IllegalPlatformAddress, sizeof(g_eoc_base_set.IllegalPlatformAddress), "%s", data.IllegalPlatformAddress.c_str());
    }
    else
    {
        memset((char *)&g_eoc_base_set, 0, sizeof(EOC_Base_Set));
    }

    DBG("g_eoc_base_set.City=%s", g_eoc_base_set.City);

    return 0;
}

int g_eoc_intersection_init(void)
{
    int ret = 0;
    DB_Intersection_t data;

    ret = db_belong_intersection_get(data);
    if(0 == ret)
    {
        snprintf(g_eoc_intersection.Guid, sizeof(g_eoc_intersection.Guid), "%s", data.Guid.c_str());
        snprintf(g_eoc_intersection.Name, sizeof(g_eoc_intersection.Name), "%s", data.Name.c_str());
        g_eoc_intersection.Type = data.Type;
        snprintf(g_eoc_intersection.PlatId, sizeof(g_eoc_intersection.PlatId), "%s", data.PlatId.c_str());
        g_eoc_intersection.XLength = data.XLength;
        g_eoc_intersection.YLength = data.YLength;
        g_eoc_intersection.LaneNumber = data.LaneNumber;
        snprintf(g_eoc_intersection.Latitude, sizeof(g_eoc_intersection.Latitude), "%s", data.Latitude.c_str());
        snprintf(g_eoc_intersection.Longitude, sizeof(g_eoc_intersection.Longitude), "%s", data.Longitude.c_str());
        g_eoc_intersection.FlagEast = data.FlagEast;
        g_eoc_intersection.FlagSouth = data.FlagSouth;
        g_eoc_intersection.FlagWest = data.FlagWest;
        g_eoc_intersection.FlagNorth = data.FlagNorth;
        g_eoc_intersection.DeltaXEast = data.DeltaXEast;
        g_eoc_intersection.DeltaYEast = data.DeltaYEast;
        g_eoc_intersection.DeltaXSouth = data.DeltaXSouth;
        g_eoc_intersection.DeltaYSouth = data.DeltaYSouth;
        g_eoc_intersection.DeltaXWest = data.DeltaXWest;
        g_eoc_intersection.DeltaYWest = data.DeltaYWest;
        g_eoc_intersection.DeltaXNorth = data.DeltaXNorth;
        g_eoc_intersection.DeltaYNorth = data.DeltaYNorth;
        g_eoc_intersection.WidthX = data.WidthX;
        g_eoc_intersection.WidthY = data.WidthY;
    }
    else
    {
        memset((char *)&g_eoc_intersection, 0, sizeof(g_eoc_intersection));
    }

    DBG("g_eoc_intersection.Name=%s", g_eoc_intersection.Name);

    return 0;
}
//融合参数
int g_eoc_fusion_para_init(void)
{
    int ret = 0;
    DB_Fusion_Para_Set_T data;

    ret = db_fusion_para_set_get(data);
    if(0 == ret)
    {
        g_eoc_fusion_para.IntersectionAreaPoint1X = data.IntersectionAreaPoint1X;
        g_eoc_fusion_para.IntersectionAreaPoint1Y = data.IntersectionAreaPoint1Y;
        g_eoc_fusion_para.IntersectionAreaPoint2X = data.IntersectionAreaPoint2X;
        g_eoc_fusion_para.IntersectionAreaPoint2Y = data.IntersectionAreaPoint2Y;
        g_eoc_fusion_para.IntersectionAreaPoint3X = data.IntersectionAreaPoint3X;
        g_eoc_fusion_para.IntersectionAreaPoint3Y = data.IntersectionAreaPoint3Y;
        g_eoc_fusion_para.IntersectionAreaPoint4X = data.IntersectionAreaPoint4X;
        g_eoc_fusion_para.IntersectionAreaPoint4Y = data.IntersectionAreaPoint4Y;
    }
    else
    {
        memset((char *)&g_eoc_fusion_para, 0, sizeof(EOC_Fusion_Para_Set));
    }

    return 0;
}

//关联设备
int g_eoc_associated_equip_init(void)
{
    int ret = 0;
    vector <DB_Associated_Equip_T> data;
    g_eoc_associated_equip.clear();
    ret = db_associated_equip_get(data);
    if(0 == ret)
    {
        EOC_Associated_Equip tmp_equip;
        for(unsigned int i=0; i<data.size(); i++)
        {
            tmp_equip.EquipType = data[i].EquipType;
            snprintf(tmp_equip.EquipCode, sizeof(tmp_equip.EquipCode), "%s", data[i].EquipCode.c_str());
            g_eoc_associated_equip.push_back(tmp_equip);
            DBG("g_eoc_associated_equip add equip[%d:]%s", tmp_equip.EquipType, tmp_equip.EquipCode);
        }
    }

    return 0;
}

//返回值：1，取到eoc配置；0，eoc配置没下发；-1，初始化数据库失败
int g_eoc_config_init(void)
{
    int ret = 0;

    if(eoc_db_table_init() < 0)
    {
        ERR("eoc_db_table_init err");
        return -1;
    }
    
    g_eoc_base_set_init();
    g_eoc_intersection_init();
    //融合参数
    g_eoc_fusion_para_init();
    //关联设备
    g_eoc_associated_equip_init();

    if(strlen(g_eoc_base_set.EocConfDataVersion) > 0)
    {
        DBG("eoc data version:%s", g_eoc_base_set.EocConfDataVersion);
        return 1;
    }
    DBG("eoc data version null");
    return 0;
}

//从~/bin/RoadsideParking.db取eoc服务器地址
int get_eoc_server_addr(string &server_path, int* server_port, string &file_server_path, int* file_server_port)
{
    *server_port = 0;
    *file_server_port = 0;
    db_parking_lot_get_cloud_addr_from_factory(server_path, server_port,file_server_path, file_server_port);
    return 0;
}




