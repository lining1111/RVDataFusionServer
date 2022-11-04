
#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "sqlite_eoc_db.h"
#include "logger.h"
#include "configure_eoc_init.h"


EOC_Base_Set g_eoc_base_set;    //基础配置
EOC_Intersection g_eoc_intersection;    //路口信息
vector<EOC_Camera_Info> g_eoc_camera_info;  //相机信息
EOC_Radar_Info g_eoc_radar_info;        //雷达信息
vector<EOC_Lane_Info> g_eoc_lane_info;  //车道信息
//EOC_Shake_Area g_eoc_shake_area;    //检测相机防抖区域
//EOC_Fusion_Area g_eoc_fusion_area;  //检测相机融合区域


#if 0
bool sort_vector_plk_space(EOC_Space a,EOC_Space b)
{
    if (strcmp(a.PlatId,b.PlatId) < 0)
    {
        return true;
    }else
    {
        return false;
    }
}
int plt_space_sort()
{
    for(unsigned int i = 0;i < g_eoc_parking_lot.size();i++)
    {
        if (g_eoc_parking_lot[i].Spaces.size() > 1)
        {
            try
            {
                DBG("排序:plkName %s",g_eoc_parking_lot[i].Name);
                stable_sort(g_eoc_parking_lot[i].Spaces.begin(),g_eoc_parking_lot[i].Spaces.end(),sort_vector_plk_space);
            }
            catch(...)
            {
                //const char* err_msg = e.what();
                ERR("SORT ERR");
                return -1;
            } 
        }
    }

    return 0;
}
bool sort_vector_plk_tag(EOC_Point_Tag a,EOC_Point_Tag b)
{
    if (strcmp(a.Tag,b.Tag) < 0)
    {
        return true;
    }else
    {
        return false;
    }
}
int point_tag_sort()
{
    for(unsigned int i = 0;i < g_eoc_parking_lot.size();i++)
    {
        for(unsigned int j=0; j<g_eoc_parking_lot[i].Spaces.size(); j++)
        {
            if (g_eoc_parking_lot[i].Spaces[j].PointTags.size() > 1)
            {
                try
                {
                    DBG("排序:PlatId %s",g_eoc_parking_lot[i].Spaces[j].PlatId);
                    stable_sort(g_eoc_parking_lot[i].Spaces[j].PointTags.begin(),g_eoc_parking_lot[i].Spaces[j].PointTags.end(),sort_vector_plk_tag);
                }
                catch(...)
                {
                    //const char* err_msg = e.what();
                    ERR("SORT ERR");
                    return -1;
                } 
            }
        }
    }

    return 0;
}
#endif
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
        snprintf(g_eoc_base_set.PlateDefault, sizeof(g_eoc_base_set.PlateDefault), "%s", data.PlateDefault.c_str());
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
        snprintf(g_eoc_base_set.CCK1Guid, sizeof(g_eoc_base_set.CCK1Guid), "%s", data.CCK1Guid.c_str());
        snprintf(g_eoc_base_set.CCK1Ip, sizeof(g_eoc_base_set.CCK1Ip), "%s", data.CCK1IP.c_str());
        snprintf(g_eoc_base_set.ErvPlatId, sizeof(g_eoc_base_set.ErvPlatId), "%s", data.ErvPlatId.c_str());
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

int g_eoc_radar_info_init(void)
{
    int ret = 0;
    DB_Radar_Info_T data;

    ret = db_radar_info_get(data);
    if(0 == ret)
    {
        snprintf(g_eoc_radar_info.Guid, sizeof(g_eoc_radar_info.Guid), "%s", data.Guid.c_str());
        snprintf(g_eoc_radar_info.Name, sizeof(g_eoc_radar_info.Name), "%s", data.Name.c_str());
        snprintf(g_eoc_radar_info.IP, sizeof(g_eoc_radar_info.IP), "%s", data.Ip.c_str());
        g_eoc_radar_info.Port = data.Port;
        g_eoc_radar_info.XThreshold = data.XThreshold;
        g_eoc_radar_info.YThreshold = data.YThreshold;
        g_eoc_radar_info.XCoordinate = data.XCoordinate;
        g_eoc_radar_info.YCoordinate = data.YCoordinate;
        g_eoc_radar_info.XRadarCoordinate = data.XRadarCoordinate;
        g_eoc_radar_info.YRadarCoordinate = data.YRadarCoordinate;
        g_eoc_radar_info.HRadarCoordinate = data.HRadarCoordinate;
        g_eoc_radar_info.CorrectiveAngle = data.CorrectiveAngle;
        g_eoc_radar_info.XOffset = data.XOffset;
        g_eoc_radar_info.YOffset = data.YOffset;
        g_eoc_radar_info.CheckDirection = data.CheckDirection;
    }
    else
    {
        memset((char *)&g_eoc_radar_info, 0, sizeof(g_eoc_radar_info));
    }

    DBG("g_eoc_radar_info.Name=%s", g_eoc_radar_info.Name);

    return 0;
}

int g_eoc_camera_info_init(void)
{
    int ret = 0;
    vector <DB_Camera_Info_T> data;
    g_eoc_camera_info.clear();
    ret = db_camera_info_get(data);
    if(0 == ret)
    {
        EOC_Camera_Info tmp_camera;
        for(unsigned int i=0; i<data.size(); i++)
        {
            snprintf(tmp_camera.Guid, sizeof(tmp_camera.Guid), "%s", data[i].Guid.c_str());
            snprintf(tmp_camera.Ip, sizeof(tmp_camera.Ip), "%s", data[i].Ip.c_str());
            snprintf(tmp_camera.Name, sizeof(tmp_camera.Name), "%s", data[i].Name.c_str());
            tmp_camera.AccessMode = data[i].AccessMode;
            tmp_camera.Type = data[i].Type;
            tmp_camera.CheckRange = data[i].CheckRange;
            tmp_camera.Direction = data[i].Direction;
            tmp_camera.PixelType = data[i].PixelType;
            tmp_camera.Delay = data[i].Delay;
            tmp_camera.IsEnable = data[i].IsEnable;
            g_eoc_camera_info.push_back(tmp_camera);
            DBG("g_eoc_camera_info add camera:%s", tmp_camera.Ip);
        }
    }

    return 0;
}
//车道信息
int g_eoc_lane_info_init()
{
    int ret = 0;
    g_eoc_lane_info.clear();
    vector <DB_Lane_Info_T> db_lane_infos;
    db_lane_infos.clear();
    ret = db_lane_info_get(db_lane_infos);
    if(ret == 0)
    {
        for(unsigned int i=0; i<db_lane_infos.size(); i++)
        {
            EOC_Lane_Info lane_info;
            lane_info.Guid = db_lane_infos[i].Guid;
            lane_info.Name = db_lane_infos[i].Name;
            lane_info.SerialNumber = db_lane_infos[i].SerialNumber;
            lane_info.DrivingDirection = db_lane_infos[i].DrivingDirection;
            lane_info.Type = db_lane_infos[i].Type;
            lane_info.Length = db_lane_infos[i].Length;
            lane_info.Width = db_lane_infos[i].Width;
            lane_info.IsEnable = db_lane_infos[i].IsEnable;
            lane_info.CameraName = db_lane_infos[i].CameraName;
        #if 0
            lane_info.LaneAreaInfos.LaneGuid = lane_info.Guid;
            lane_info.LaneAreaInfos.LaneLineInfos.clear();
            lane_info.LaneAreaInfos.IdentificationAreas.clear();
            //车道线
            vector <DB_Lane_Line_T> db_lane_lines;
            db_lane_lines.clear();
            db_lane_line_get_by_guid(db_lane_lines, lane_info.Guid.c_str());
            for(unsigned int line_i=0; line_i<db_lane_lines.size(); line_i++)
            {
                EOC_Lane_Line lane_line_info;
                lane_line_info.LaneLineGuid = db_lane_lines[line_i].LaneLineGuid;
                lane_line_info.Type = db_lane_lines[line_i].Type;
                lane_line_info.Color = db_lane_lines[line_i].Color;
                lane_line_info.CoordinateSet = db_lane_lines[line_i].CoordinateSet;
                lane_info.LaneAreaInfos.LaneLineInfos.push_back(lane_line_info);
            }
            //车道识别区
            vector <DB_Lane_Identif_Area_T> db_identif_areas;
            db_identif_areas.clear();
            db_lane_identif_area_get_by_guid(db_identif_areas, lane_info.Guid.c_str());
            for(unsigned int identif_i=0; identif_i<db_identif_areas.size(); identif_i++)
            {
                EOC_Identif_Area identif_area;
                identif_area.Guid = db_identif_areas[identif_i].IdentifAreaGuid;
                identif_area.Type = db_identif_areas[identif_i].Type;
                identif_area.IdentificationAreaPoints.clear();
                //车道识别区点
                vector <DB_Lane_Identif_Area_Point_T> db_identif_points;
                db_identif_points.clear();
                db_lane_identif_point_get_byguid(db_identif_points, identif_area.Guid.c_str());
                for(unsigned int point_i=0; point_i<db_identif_points.size(); point_i++)
                {
                    EOC_Identif_Area_Point identif_point;
                    identif_point.SerialNumber = db_identif_points[point_i].SerialNumber;
                    identif_point.XPixel = db_identif_points[point_i].XPixel;
                    identif_point.YPixel = db_identif_points[point_i].YPixel;
                    identif_area.IdentificationAreaPoints.push_back(identif_point);
                }
                lane_info.LaneAreaInfos.IdentificationAreas.push_back(identif_area);
            }
        #endif
            g_eoc_lane_info.push_back(lane_info);
        }
    }

    return 0;
}
#if 0
//检测相机防抖区域
int g_eoc_shake_area_init()
{
    int ret = 0;
    DB_Detect_Shake_Area_T db_area;
    ret = db_detect_shake_area_get(db_area);
    if(ret == 0)
    {
        g_eoc_shake_area.Guid = db_area.Guid;
        g_eoc_shake_area.XOffset = db_area.XOffset;
        g_eoc_shake_area.YOffset = db_area.YOffset;
        g_eoc_shake_area.ShakePoints.clear();
        //防抖区点
        vector <DB_Detect_Shake_Point_T> db_points;
        db_points.clear();
        db_detect_shake_point_get_byguid(db_points, g_eoc_shake_area.Guid.c_str());
        for(unsigned int i=0; i<db_points.size(); i++)
        {
            EOC_Shake_Point shake_point;
            shake_point.SerialNumber = db_points[i].SerialNumber;
            shake_point.XCoordinate = db_points[i].XCoordinate;
            shake_point.YCoordinate = db_points[i].YCoordinate;
            shake_point.XPixel = db_points[i].XPixel;
            shake_point.YPixel = db_points[i].YPixel;
            g_eoc_shake_area.ShakePoints.push_back(shake_point);
        }
    }
    else
    {
        g_eoc_shake_area.Guid = "";
        g_eoc_shake_area.XOffset = 0.0;
        g_eoc_shake_area.YOffset = 0.0;
        g_eoc_shake_area.ShakePoints.clear();
    }

    return 0;
}

//检测相机融合区域
int g_eoc_fusion_area_init()
{
    int ret = 0;
    DB_Detect_Fusion_Area_T db_area;
    ret = db_detect_fusion_area_get(db_area);
    if(ret == 0)
    {
        g_eoc_fusion_area.Guid = db_area.Guid;
        g_eoc_fusion_area.PerspectiveTransFa[0][0] = db_area.PerspectiveTransFa00;
        g_eoc_fusion_area.PerspectiveTransFa[0][1] = db_area.PerspectiveTransFa01;
        g_eoc_fusion_area.PerspectiveTransFa[0][2] = db_area.PerspectiveTransFa02;
        g_eoc_fusion_area.PerspectiveTransFa[1][0] = db_area.PerspectiveTransFa10;
        g_eoc_fusion_area.PerspectiveTransFa[1][1] = db_area.PerspectiveTransFa11;
        g_eoc_fusion_area.PerspectiveTransFa[1][2] = db_area.PerspectiveTransFa12;
        g_eoc_fusion_area.PerspectiveTransFa[2][0] = db_area.PerspectiveTransFa20;
        g_eoc_fusion_area.PerspectiveTransFa[2][1] = db_area.PerspectiveTransFa21;
        g_eoc_fusion_area.PerspectiveTransFa[2][2] = db_area.PerspectiveTransFa22;
        g_eoc_fusion_area.XDistance = db_area.XDistance;
        g_eoc_fusion_area.YDistance = db_area.YDistance;
        g_eoc_fusion_area.HDistance = db_area.HDistance;
        g_eoc_fusion_area.MPPW = db_area.MPPW;
        g_eoc_fusion_area.MPPH = db_area.MPPH;
        g_eoc_fusion_area.FusionPoints.clear();
        //融合区点
        vector <DB_Detect_Fusion_Point_T> db_points;
        db_points.clear();
        db_detect_fusion_p_get_by_guid(db_points, g_eoc_fusion_area.Guid.c_str());
        for(unsigned int i=0; i<db_points.size(); i++)
        {
            EOC_Fusion_Point fusion_point;
            fusion_point.SerialNumber = db_points[i].SerialNumber;
            fusion_point.XCoordinate = db_points[i].XCoordinate;
            fusion_point.YCoordinate = db_points[i].YCoordinate;
            fusion_point.XPixel = db_points[i].XPixel;
            fusion_point.YPixel = db_points[i].YPixel;
            g_eoc_fusion_area.FusionPoints.push_back(fusion_point);
        }
    }
    else
    {
        g_eoc_fusion_area.Guid = "";
        g_eoc_fusion_area.FusionPoints.clear();
        memset(g_eoc_fusion_area.PerspectiveTransFa,0,sizeof(g_eoc_fusion_area.PerspectiveTransFa));
        g_eoc_fusion_area.XDistance = 0.0;
        g_eoc_fusion_area.YDistance = 0.0;
        g_eoc_fusion_area.HDistance = 0.0;
        g_eoc_fusion_area.MPPW = 0.0;
        g_eoc_fusion_area.MPPH = 0.0;
    }

    return 0;
}
#endif
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
    g_eoc_camera_info_init();
    g_eoc_radar_info_init();
    //车道信息
    g_eoc_lane_info_init();

    //检测相机防抖区域
//    g_eoc_shake_area_init();
    //检测相机融合区域
//    g_eoc_fusion_area_init();

    if(strlen(g_eoc_base_set.EocConfDataVersion) > 0)
    {
        DBG("eoc data version:%s", g_eoc_base_set.EocConfDataVersion);
        return 1;
    }
    DBG("eoc data version null");
    return 0;
}

//通过相机guid获取相机详细配置
//返回值：1 查找到，其他 未查到
int get_camera_detail_set(const char* camera_guid, EOC_Camera_Detail_info &data)
{
    int ret = 0;
    DB_Camera_Detail_Conf_T db_data;
    ret = db_camera_detail_set_get_by_guid((char*)camera_guid, db_data);
    if(ret != 1)
    {
        return 0;
    }
    else
    {
        snprintf(data.Guid, sizeof(data.Guid), "%s", db_data.Guid.c_str());
        snprintf(data.DataVersion, sizeof(data.DataVersion), "%s", db_data.DataVersion.c_str());
        data.Brightness = db_data.Brightness;
        data.Contrast = db_data.Contrast;
        data.Saturation = db_data.Saturation;
        data.Acutance = db_data.Acutance;
        data.MaximumGain = db_data.MaximumGain;
        data.ExposureTime = db_data.ExposureTime;
        data.DigitalNoiseReduction = db_data.DigitalNoiseReduction;
        data.DigitalNoiseReductionLevel = db_data.DigitalNoiseReductionLevel;
        data.WideDynamic = db_data.WideDynamic;
        data.WideDynamicLevel = db_data.WideDynamicLevel;
        data.BitStreamType = db_data.BitStreamType;
        data.Resolution = db_data.Resolution;
        data.Fps = db_data.Fps;
        data.CodecType = db_data.CodecType;
        data.ImageQuality = db_data.ImageQuality;
        data.BitRateUpper = db_data.BitRateUpper;
        data.IsShowName = db_data.IsShowName;
        data.IsShowDate = db_data.IsShowDate;
        data.IsShowWeek = db_data.IsShowWeek;
        data.TimeFormat = db_data.TimeFormat;
        snprintf(data.DateFormat, sizeof(data.DateFormat), "%s", db_data.DateFormat.c_str());
        snprintf(data.ChannelName, sizeof(data.ChannelName), "%s", db_data.ChannelName.c_str());
        data.FontSize = db_data.FontSize;
        snprintf(data.FontColor, sizeof(data.FontColor), "%s", db_data.FontColor.c_str());
        data.Transparency = db_data.Transparency;
        data.Flicker = db_data.Flicker;
    }
    
    return 1;
}

//取相机类型
//返回值：1=检测相机，2=识别相机，0没查到相机
int get_camera_type(const char* camera_ip)
{
    unsigned int i = 0;

    for(i=0; i<g_eoc_camera_info.size(); i++)
    {
        if(strcmp(camera_ip, g_eoc_camera_info[i].Ip) == 0)
        {
            return g_eoc_camera_info[i].Type;
        }
    }
    ERR("not find camera %s", camera_ip);
    return 0;
}

//取相机检测范围
//返回值：1=远端，2=近端，0没查到相机
int get_camera_check_range(const char* camera_ip)
{
    unsigned int i = 0;

    for(i=0; i<g_eoc_camera_info.size(); i++)
    {
        if(strcmp(camera_ip, g_eoc_camera_info[i].Ip) == 0)
        {
            return g_eoc_camera_info[i].CheckRange;
        }
    }
    ERR("not find camera %s", camera_ip);
    return 0;
}

//根据相机guid取相机ip
//返回值：相机ip
string get_camera_ip(const char* camera_guid)
{
    unsigned int i = 0;

    for(i=0; i<g_eoc_camera_info.size(); i++)
    {
        if(strcmp(camera_guid, g_eoc_camera_info[i].Guid) == 0)
        {
            return string(g_eoc_camera_info[i].Ip);
        }
    }
    ERR("not find camera %s", camera_guid);
    return "";
}
//根据相机ip取相机guid
//返回值：相机guid
string get_camera_guid(const char* camera_ip)
{
    unsigned int i = 0;

    for(i=0; i<g_eoc_camera_info.size(); i++)
    {
        if(strcmp(camera_ip, g_eoc_camera_info[i].Ip) == 0)
        {
            return string(g_eoc_camera_info[i].Guid);
        }
    }
    ERR("not find camera %s", camera_ip);
    return "";
}


//根据矩阵控制器guid取ip
//返回值：0=取到ip，-1=输入guid错误
int get_controller_ip(string &guid, string &ip)
{
    if(strcmp(guid.c_str(), g_eoc_base_set.CCK1Guid)==0) 
    {
        if(strlen(g_eoc_base_set.CCK1Ip)>0)
        {
            ip = g_eoc_base_set.CCK1Ip;
        }
        else
        {
            ip = "192.168.8.2"; //矩阵控制器默认ip
        }
        return 0;
    }
    else
    {
        DBG("in guid:%s, local controllerguid:%s", guid.c_str(), g_eoc_base_set.CCK1Guid);
        return -1;
    }
}

int get_camera_shake_area_info(const char* camera_guid, vector <EOC_Shake_Area> &data)
{
    int ret = 0;
    data.clear();
    vector <DB_Detect_Shake_Area_T> db_shake_area;
    db_shake_area.clear();
    ret = db_detect_shake_area_get(db_shake_area, camera_guid);
    for(unsigned int i=0; i<db_shake_area.size(); i++)
    {
        EOC_Shake_Area shake_area;
        shake_area.Guid = db_shake_area[i].Guid;
        shake_area.XOffset = db_shake_area[i].XOffset;
        shake_area.YOffset = db_shake_area[i].YOffset;
        shake_area.ShakePoints.clear();
        //防抖区点
        vector <DB_Detect_Shake_Point_T> db_points;
        db_points.clear();
        db_detect_shake_point_get_byguid(db_points, shake_area.Guid.c_str());
        for(unsigned int j=0; j<db_points.size(); j++)
        {
            EOC_Shake_Point shake_point;
            shake_point.SerialNumber = db_points[j].SerialNumber;
            shake_point.XCoordinate = db_points[j].XCoordinate;
            shake_point.YCoordinate = db_points[j].YCoordinate;
            shake_point.XPixel = db_points[j].XPixel;
            shake_point.YPixel = db_points[j].YPixel;
            shake_area.ShakePoints.push_back(shake_point);
        }
        data.push_back(shake_area);
    }

    return 0;
}
int get_camera_fusion_area_info(const char* camera_guid, vector <EOC_Fusion_Area> &data)
{
    int ret = 0;
    data.clear();
    vector <DB_Detect_Fusion_Area_T> db_fusion_data;
    db_fusion_data.clear();
    ret = db_detect_fusion_area_get(db_fusion_data, camera_guid);
    for(unsigned int i=0; i<db_fusion_data.size(); i++)
    {
        EOC_Fusion_Area fusion_area;
        fusion_area.Guid = db_fusion_data[i].Guid;
        fusion_area.PerspectiveTransFa[0][0] = db_fusion_data[i].PerspectiveTransFa00;
        fusion_area.PerspectiveTransFa[0][1] = db_fusion_data[i].PerspectiveTransFa01;
        fusion_area.PerspectiveTransFa[0][2] = db_fusion_data[i].PerspectiveTransFa02;
        fusion_area.PerspectiveTransFa[1][0] = db_fusion_data[i].PerspectiveTransFa10;
        fusion_area.PerspectiveTransFa[1][1] = db_fusion_data[i].PerspectiveTransFa11;
        fusion_area.PerspectiveTransFa[1][2] = db_fusion_data[i].PerspectiveTransFa12;
        fusion_area.PerspectiveTransFa[2][0] = db_fusion_data[i].PerspectiveTransFa20;
        fusion_area.PerspectiveTransFa[2][1] = db_fusion_data[i].PerspectiveTransFa21;
        fusion_area.PerspectiveTransFa[2][2] = db_fusion_data[i].PerspectiveTransFa22;
        fusion_area.XDistance = db_fusion_data[i].XDistance;
        fusion_area.YDistance = db_fusion_data[i].YDistance;
        fusion_area.HDistance = db_fusion_data[i].HDistance;
        fusion_area.MPPW = db_fusion_data[i].MPPW;
        fusion_area.MPPH = db_fusion_data[i].MPPH;
        fusion_area.FusionPoints.clear();
        //融合区点
        vector <DB_Detect_Fusion_Point_T> db_points;
        db_points.clear();
        db_detect_fusion_p_get_by_guid(db_points, fusion_area.Guid.c_str());
        for(unsigned int j=0; j<db_points.size(); j++)
        {
            EOC_Fusion_Point fusion_point;
            fusion_point.SerialNumber = db_points[j].SerialNumber;
            fusion_point.XCoordinate = db_points[j].XCoordinate;
            fusion_point.YCoordinate = db_points[j].YCoordinate;
            fusion_point.XPixel = db_points[j].XPixel;
            fusion_point.YPixel = db_points[j].YPixel;
            fusion_area.FusionPoints.push_back(fusion_point);
        }
        data.push_back(fusion_area);
    }

    return 0;
}
int get_camera_lane_area_info(const char* camera_guid, vector <EOC_Lane_Area> &data)
{
    int ret = 0;
    data.clear();
    //车道线
    vector <DB_Lane_Line_T> db_lane_lines;
    db_lane_lines.clear();
    db_lane_line_get_by_camera(db_lane_lines, camera_guid);
    for(unsigned int i=0; i<db_lane_lines.size(); i++)
    {
        unsigned int lane_i = 0;
        for(lane_i=0; lane_i<data.size(); lane_i++)
        {
            if(db_lane_lines[i].LaneGuid == data[lane_i].LaneGuid)
            {
                EOC_Lane_Line lane_line;
                lane_line.LaneLineGuid = db_lane_lines[i].LaneLineGuid;
                lane_line.Type = db_lane_lines[i].Type;
                lane_line.Color = db_lane_lines[i].Color;
                lane_line.CoordinateSet = db_lane_lines[i].CoordinateSet;
                data[lane_i].LaneLineInfos.push_back(lane_line);
                break;
            }
        }
        if(lane_i == data.size())
        {
            EOC_Lane_Area tmp_lane_area;
            tmp_lane_area.LaneGuid = db_lane_lines[i].LaneGuid;
            tmp_lane_area.LaneLineInfos.clear();
            tmp_lane_area.IdentificationAreas.clear();
            EOC_Lane_Line lane_line;
            lane_line.LaneLineGuid = db_lane_lines[i].LaneLineGuid;
            lane_line.Type = db_lane_lines[i].Type;
            lane_line.Color = db_lane_lines[i].Color;
            lane_line.CoordinateSet = db_lane_lines[i].CoordinateSet;
            tmp_lane_area.LaneLineInfos.push_back(lane_line);
            data.push_back(tmp_lane_area);
        }     
    }
    //车道识别区
    vector <DB_Lane_Identif_Area_T> db_identif_areas;
    db_identif_areas.clear();
    db_lane_identif_area_get_by_camera(db_identif_areas, camera_guid);
    for(unsigned int i=0; i<db_identif_areas.size(); i++)
    {
        unsigned int lane_i = 0;
        for(lane_i=0; lane_i<data.size(); lane_i++)
        {
            if(db_identif_areas[i].LaneGuid == data[lane_i].LaneGuid)
            {
                EOC_Identif_Area identif_area;
                identif_area.Guid = db_identif_areas[i].IdentifAreaGuid;
                identif_area.Type = db_identif_areas[i].Type;
                identif_area.IdentificationAreaPoints.clear();
                //车道识别区点
                vector <DB_Lane_Identif_Area_Point_T> db_identif_points;
                db_identif_points.clear();
                db_lane_identif_point_get_byguid(db_identif_points, identif_area.Guid.c_str());
                for(unsigned int point_i=0; point_i<db_identif_points.size(); point_i++)
                {
                    EOC_Identif_Area_Point identif_point;
                    identif_point.SerialNumber = db_identif_points[point_i].SerialNumber;
                    identif_point.XPixel = db_identif_points[point_i].XPixel;
                    identif_point.YPixel = db_identif_points[point_i].YPixel;
                    identif_area.IdentificationAreaPoints.push_back(identif_point);
                }
                data[lane_i].IdentificationAreas.push_back(identif_area);
                break;
            }
        }
        if(lane_i == data.size())
        {
            EOC_Lane_Area tmp_lane_area;
            tmp_lane_area.LaneGuid = db_identif_areas[i].LaneGuid;
            tmp_lane_area.LaneLineInfos.clear();
            tmp_lane_area.IdentificationAreas.clear();
            EOC_Identif_Area identif_area;
            identif_area.Guid = db_identif_areas[i].IdentifAreaGuid;
            identif_area.Type = db_identif_areas[i].Type;
            identif_area.IdentificationAreaPoints.clear();
            //车道识别区点
            vector <DB_Lane_Identif_Area_Point_T> db_identif_points;
            db_identif_points.clear();
            db_lane_identif_point_get_byguid(db_identif_points, identif_area.Guid.c_str());
            for(unsigned int point_i=0; point_i<db_identif_points.size(); point_i++)
            {
                EOC_Identif_Area_Point identif_point;
                identif_point.SerialNumber = db_identif_points[point_i].SerialNumber;
                identif_point.XPixel = db_identif_points[point_i].XPixel;
                identif_point.YPixel = db_identif_points[point_i].YPixel;
                identif_area.IdentificationAreaPoints.push_back(identif_point);
            }
            tmp_lane_area.IdentificationAreas.push_back(identif_area);
            data.push_back(tmp_lane_area);
            
        }
    }

    return 0;
}

//根据相机ip取相机所有区域信息，包括防抖区，融合区，车道区
//返回值：-1错误，未找到相机对应区域配置信息   ；0正确
int get_camera_area_set_info(const char* ip, EOC_Camera_Area_Info &data)
{
    int ret = 0;
    if(strlen(ip) < 7)
    {
        ERR("ip %s err", ip);
        return -1;
    }
    string camera_guid = get_camera_guid(ip);
    if(camera_guid.length()<=0)
    {
        ERR("get camera guid err");
        return -1;
    }
    data.CameraGuid = camera_guid;

    //防抖区
    get_camera_shake_area_info(camera_guid.c_str(), data.ShakeAreas);
    //融合区
    get_camera_fusion_area_info(camera_guid.c_str(), data.FusionAreas);
    //车道区域
    get_camera_lane_area_info(camera_guid.c_str(), data.LaneAreas);

    return 0;
}



