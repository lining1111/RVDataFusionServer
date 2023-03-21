//
// Created by lining on 2023/3/15.
//

#include "transform.h"

Perceptron PerceptronGet(ObjMix obj, int64_t timestamp) {
    Perceptron ret;
    ret.set_is_tracker(true);//物体是否被跟踪---默认为真
//    ret.set_object_confidence();//目标置信度---?
    ret.set_lane_id(obj.laneCode);//目标所在车道id---车道号
    ret.set_object_class_type(obj.objType);//目标大类类型---目标类型
    ret.set_object_id(obj.objID);//单杆目标Id---目标ID
    // ret.set_allocated_point3f();//目标的几何中心相对传感器坐标位置---可能与rvWayObject内的成员有关
//   ret.set_allocated_point4f();//图像坐标位置---可能与rvWayObject内的成员有关
    ret.set_object_speed(obj.speed);//目标速度---速度
//    ret.set_allocated_speed3f();//目标分轴速度---?
//    ret.set_object_acceleration();目标加速度---?
//    ret.set_allocated_target_size();//目标尺寸---?
//    ret.set_allocated_point_gps();//目标经纬度坐标---?
    ret.set_object_ns(obj.latitude);//南北纬---纬度
    ret.set_object_we(obj.longitude);//东西经---经度
//    ret.set_object_direction();//车辆车头朝向角度，与正北偏角---?
    ret.set_object_heading(obj.angle);//行驶方向朝向航向角---航角度
//    ret.set_is_head_tail();//车头还是车尾---?
//    ret.set_lane_type();//目标所在车道类型---?
    ret.set_plate_num(obj.plates);//目标为车辆时，为车牌号---车牌号
    ret.set_objects_identity(to_string(obj.objID));//全域目标id---目标ID 也可能不一样，数据类型不一致
    ret.set_fuel_type(obj.carType);//目标燃油类型---车辆类型
//    ret.set_allocated_accel_4way();//目标四轴加速度---?
    ret.set_obj_time_stamp(timestamp);//目标时间戳-毫秒数
//    ret.set_ptc_sourcetype();//目标数据来源---?
//    ret.set_allocated_ptc_time_stamp();//目标时间戳-带格式---?
//    ret.set_allocated_ptc_gps_cfd();//目标经纬度坐标置信度---?
//    ret.set_ptc_tran_state();//目标车辆档位状态---?
    ret.set_ptc_angle(obj.angle);//目标转向角---航角度
//    ret.set_allocated_ptc_motino_cfd();//目标运行状态置信度---?
    ret.set_ptc_veh_type(obj.carType);//目标车辆类型---车辆类型
//    ret.set_allocated_ptc_size_cfd();//目标尺寸置信度---?
//    ret.set_ptc_exttype();//目标类型扩展---?
//    ret.set_ptc_exttype_cfd();//目标类型置信度---?
//    ret.set_allocated_ptc_accel_4way_cfd();//目标四轴加速度置信度---?
//    ret.set_ptc_status_duration();//目标状态点偏差---?
//    ret.set_ptc_satellite();//目标所在点卫星数量---?
//    ret.set_ptc_regionid();//目标所在点区域id---?
//    ret.set_ptc_nodeid();//目标所在点区域内节点id---?
    ret.set_lane_id(obj.laneCode);//目标所在点车道id---车道号
//    ret.set_ptc_link_name();//目标所在点道路名称---?
//    ret.set_ptc_link_width();//目标所在点道路宽度---?
//    ret.set_ptc_veh_plate_type();//车牌类型---?
//    ret.set_ptc_veh_plate_color(obj.plateColor);//车牌颜色---车牌颜色 但数据类型不一致

    return ret;
}

LaneJamSenseParams LaneJamSenseParamsGet(OneFlowData obj, int64_t timestamp) {
    LaneJamSenseParams ret;
    ret.set_lane_id(obj.laneCode);//车道id---车道编号
    ret.set_lane_types(obj.flowDirection);//车道类型---流向方向
//    ret.set_lane_sense_len();//车道长度---?
    ret.set_lane_direction(obj.laneDirection);//车道方向---车道方向
    ret.set_lane_avg_speed(obj.inAverageSpeed);//车道内车辆的平均速度---入口平均速度
    ret.set_lane_veh_num(obj.inCars + obj.outCars);//该车道不区分车型机动车总流量---进入数量+驶出数量
//    ret.set_lane_space_occupancy();//车道空间占有率---?
    ret.set_lane_queue_len(obj.queueLen);//车道排队长度---排队长度
    ret.set_lane_count_time(timestamp);//车道统计时间
    ret.set_lane_count_flow(obj.inCars + obj.outCars);//车道统计车数量---进入数量+驶出数量
    ret.set_lane_is_count(true);//是否统计成功
//    ret.set_lane_ave_distance();//车道平均车间距---?
//    ret.set_lane_cur_distance();//车道实时车间距---?
//    ret.set_lane_time_occupancy();//车道时间占有率---?
//    ret.set_allocated_lane_entre_info();//进入车道的交通信息---?
//    ret.set_allocated_lane_end_info();//离开车道的交通信息---?
//    ret.set_lane_num();//车道数量----17以下襄阳项目定义---?
//    ret.set_lane_no();//车道编号---?
//    ret.set_lane_peron_volume();//该车道行人流量---?
//    ret.set_lane_no_motor_volume();//该车道非机动车流量---?
//    ret.set_lane_minmotor_volume();//该车道小车总流量---?
//    ret.set_lane_medmotor_volume();//该车道中车流量---?
//    ret.set_lane_maxmotor_volume();//该车道大车流量---?
//    ret.set_lane_pcu();//该车道交通当量---?
    ret.set_lane_avspeed(obj.outAverageSpeed);//该车道平均速度---出口平均速度
//    ret.set_lane_headway();//该车道平均车头时距---?
//    ret.set_lane_gap();//该车道平均车身间距---?
//    ret.set_lane_avdistance();//该车道平均车距---?
//    ret.set_lane_avstop();//该车道平均停车次数---?
//    ret.set_lane_speed85();//该车道85%以上的车辆的最高速度---?
    ret.set_lane_queuelength(obj.queueLen);//该车道来向车道排队长度---排队长度
//    ret.set_lane_stopline();//该车道停止线---?

    return ret;
}
