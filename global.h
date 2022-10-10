//
// Created by lining on 10/10/22.
//

#ifndef _GLOBAL_H
#define _GLOBAL_H

#include <server/FusionServer.h>
#include <multiView/MultiViewServer.h>
#include <client/FusionClient.h>

typedef struct {
    FusionServer *server;
    MultiViewServer *multiViewServer;
    FusionClient *client;
    bool isRun;
} Local;


#endif //_GLOBAL_H
