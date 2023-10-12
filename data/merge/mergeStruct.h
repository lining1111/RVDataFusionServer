//
// Created by lining on 6/30/22.
//

#ifndef _MERGESTRUCT_H
#define _MERGESTRUCT_H

#include "lib/merge.h"

void OBJECT_INFO_T2ObjTarget(OBJECT_INFO_T &objectInfoT, ObjTarget &objTarget);

void ObjTarget2OBJECT_INFO_T(ObjTarget &objTarget, OBJECT_INFO_T &objectInfoT);

void OBJECT_INFO_T2OBJECT_INFO_NEW(OBJECT_INFO_T &objectInfoT, OBJECT_INFO_NEW &objectInfoNew);

void OBJECT_INFO_NEW2ObjMix(OBJECT_INFO_NEW &objectInfoNew, ObjMix &objMix);



#endif //_MERGESTRUCT_H
