//
// Created by lining on 10/10/22.
//

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H
#include "localBussiness/localBusiness.h"

#include <string>
#include <vector>
#include <json/json.h>

using namespace std;

class ServerClientInfo{
public:
   string ip;
   int direction;
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class ServerInfo{
public:
    string ip;
    int port;
    vector<ServerClientInfo> clients;
    vector<string> tasks;
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class CliInfo{
public:
    string serverIp;
    int serverPort;
    bool isConnect;
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class TimerTasksInfo{
public:
    vector<string> names;
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

class LocalInfo{
public:
    vector<ServerInfo> serverInfos;
    vector<CliInfo> cliInfos;
    TimerTasksInfo timerTasksInfo;
public:
    bool JsonMarshal(Json::Value &out);

    bool JsonUnmarshal(Json::Value in);
};

int HttpServerInit(int port, LocalBusiness *local);

#endif //_HTTPSERVER_H
