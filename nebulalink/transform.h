//
// Created by lining on 2023/3/15.
//

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "nebulalink.perceptron3.0.5.pb.h"
#include "../common/common.h"

using namespace nebulalink::perceptron3;
using namespace common;
/**
 * 从 感知共享消息-结构化数据 PerceptronSet内部的各个结构体元素出发，找到可能的对应关系
 */

//感知参与者列表 Perceptron 对应2.1.1监控数据上传中的视频识别目标列表 FusionData--lstObjTarget--ObjMix
Perceptron PerceptronGet(ObjMix obj, int64_t timestamp);

//lane级道路信息 LaneJamSenseParams 对应 2.1.5 路口交通流向上传 TrafficFlow---flowData
LaneJamSenseParams LaneJamSenseParamsGet(OneFlowData obj,int64_t timestamp);

//道路级link 信息列表 LinkJamSenseParams 暂无

#endif //TRANSFORM_H
