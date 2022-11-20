//
// Created by lining on 11/20/22.
//

#ifndef _FINDONEFRAME_H
#define _FINDONEFRAME_H

#include "DataUnit.h"
#include "common/common.h"

/**
 * 寻找同一帧数据的方法类
 */

namespace FindOneFrame {
    //
    typedef void(*PMultiViewCarTracks)(DataUnit<common::MultiViewCarTrack, common::MultiViewCarTracks> &);

    int FindMultiViewCarTracks(DataUnit<common::MultiViewCarTrack, common::MultiViewCarTracks> &dataUnit,
                               unsigned int cache, uint64_t toCacheCha, PMultiViewCarTracks task,
                               bool isFront = true);

    typedef void(*PTrafficFlows)(DataUnit<common::TrafficFlow, common::TrafficFlows> &);

    int FindTrafficFlows(DataUnit<common::TrafficFlow, common::TrafficFlows> &dataUnit,
                         unsigned int cache, uint64_t toCacheCha, PTrafficFlows task, bool isFront = true);

    typedef void(*PCrossTrafficJamAlarm)(DataUnit<common::CrossTrafficJamAlarm, common::CrossTrafficJamAlarm> &);

    int FindCrossTrafficJamAlarm(DataUnit<common::CrossTrafficJamAlarm, common::CrossTrafficJamAlarm> &dataUnit,
                                 unsigned int cache, uint64_t toCacheCha, PCrossTrafficJamAlarm task,
                                 bool isFront = true);

    typedef void(*PLineupInfoGather)(DataUnit<common::LineupInfo, common::LineupInfoGather> &);

    int FindLineupInfoGather(DataUnit<common::LineupInfo, common::LineupInfoGather> &dataUnit,
                             unsigned int cache, uint64_t toCacheCha, PLineupInfoGather task,
                             bool isFront = true);
};


#endif //_FINDONEFRAME_H
