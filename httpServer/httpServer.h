//
// Created by lining on 10/10/22.
//

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H
#include "global.h"

#include <string>
#include <vector>

#ifdef x86
#include <jsoncpp/json/json.h>
#else
#include <json/json.h>
#endif

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

int HttpServerInit(int port,Local *local);

#endif //_HTTPSERVER_H
