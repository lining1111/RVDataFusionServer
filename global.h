//
// Created by lining on 10/10/22.
//

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <server/FusionServer.h>
#include <client/FusionClient.h>

typedef struct {
    std::vector<FusionServer *> serverList;
    std::vector<FusionClient *> clientList;
    bool isRun;
} Local;


#endif //_GLOBAL_H
