//
// Created by lining on 8/11/22.
//

#ifndef _DB_H
#define _DB_H

#include <string>

using namespace std;

typedef struct {
    string ip;
    int port;
} EocConfigCloud;

extern string eocConfigDB;
void getCloudConfigFromDb(EocConfigCloud *config);

#endif //_DB_H
