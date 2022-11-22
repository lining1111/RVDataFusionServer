//
// Created by lining on 10/10/22.
//

#include "httpServer.h"
#include <httplib.h>
#include "log/Log.h"

using namespace z_log;

httplib::Server svr;
static LocalBusiness *mLocal = nullptr;

void GetLocalInfoCB(const httplib::Request &req, httplib::Response &resp) {
    resp.set_header("Content-Type", "plain/text");
    resp.status = 200;

    LocalInfo localInfo;

    for (auto iter:mLocal->serverList) {
        ServerInfo item;
        item.ip = "localhost";
        item.port = iter.second->port;
        for (auto iter1:iter.second->vectorClient) {
            ServerClientInfo item1;
            item1.ip = string(inet_ntoa(iter1->clientAddr.sin_addr));
            item1.direction = iter1->direction;
            item.clients.push_back(item1);
        }

        map<string, Timer *>::iterator iter2;
        for (iter2 = iter.second->timerTasks.begin(); iter2 != iter.second->timerTasks.end(); iter2++) {
            item.tasks.push_back(iter2->first);
        }

        localInfo.serverInfos.push_back(item);
    }

    for (auto iter:mLocal->clientList) {
        CliInfo item;
        item.serverIp = iter.second->server_ip;
        item.serverPort = iter.second->server_port;
        localInfo.cliInfos.push_back(item);
    }


    map<string, Timer *>::iterator iter;
    for (iter = mLocal->timerTasks.begin(); iter != mLocal->timerTasks.end(); iter++) {
        localInfo.timerTasksInfo.names.push_back(iter->first);
    }


    string jsonStr;
    Json::FastWriter fastWriter;
    Json::Value root;
    localInfo.JsonMarshal(root);
    jsonStr = fastWriter.write(root);

    resp.set_content(jsonStr, "plain/text");
}


void MThread(void *p, int port) {
    httplib::Server *svr = (httplib::Server *) p;
    Info("开启web服务：%d", port);
    svr->listen("0.0.0.0", port);
}


int HttpServerInit(int port, LocalBusiness *local) {
    mLocal = local;

    svr.Get("/getLocalInfo", GetLocalInfoCB);

//    svr.listen("0.0.0.0", port);
    thread mThread = thread(MThread, &svr, port);
    mThread.detach();

}

bool ServerClientInfo::JsonMarshal(Json::Value &out) {
    out["ip"] = this->ip;
    out["direction"] = this->direction;

    return true;
}

bool ServerClientInfo::JsonUnmarshal(Json::Value in) {
    this->ip = in["ip"].asString();
    this->direction = in["direction"].asInt();

    return true;
}

bool ServerInfo::JsonMarshal(Json::Value &out) {
    out["ip"] = this->ip;
    out["port"] = this->port;

    Json::Value clients = Json::arrayValue;
    if (!this->clients.empty()) {
        for (auto iter:this->clients) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                clients.append(item);
            }
        }
    } else {
        clients.resize(0);
    }
    out["clients"] = clients;

    Json::Value tasks = Json::arrayValue;
    if (!this->tasks.empty()) {
        for (auto iter:this->tasks) {
            Json::Value item;
            item["task"] = iter;
            tasks.append(item);
        }
    } else {
        tasks.resize(0);
    }
    out["tasks"] = tasks;

    return true;
}

bool ServerInfo::JsonUnmarshal(Json::Value in) {
    this->ip = in["ip"].asString();
    this->port = in["port"].asInt();

    if (in["clients"].isArray()) {
        Json::Value clients = in["clients"];
        for (auto iter:clients) {
            ServerClientInfo item;
            if (item.JsonUnmarshal(iter)) {
                this->clients.push_back(item);
            }
        }
    }
    if (in["tasks"].isArray()) {
        Json::Value tasks = in["tasks"];
        for (auto iter:tasks) {
            string item;
            item = iter.asString();
            this->tasks.push_back(item);
        }
    }

    return true;
}

bool CliInfo::JsonMarshal(Json::Value &out) {
    out["serverIp"] = this->serverIp;
    out["serverPort"] = this->serverPort;
    out["isConnect"] = this->isConnect;
    return true;
}

bool CliInfo::JsonUnmarshal(Json::Value in) {
    this->serverIp = in["serverIp"].asString();
    this->serverPort = in["serverPort"].asInt();
    this->isConnect = in["isConnect"].asBool();

    return true;
}

bool LocalInfo::JsonMarshal(Json::Value &out) {
    Json::Value serverInfos = Json::arrayValue;
    if (!this->serverInfos.empty()) {
        for (auto iter:this->serverInfos) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                serverInfos.append(item);
            }
        }
    } else {
        serverInfos.resize(0);
    }
    out["serverInfos"] = serverInfos;

    Json::Value cliInfos = Json::arrayValue;
    if (!this->cliInfos.empty()) {
        for (auto iter:this->cliInfos) {
            Json::Value item;
            if (iter.JsonMarshal(item)) {
                cliInfos.append(item);
            }
        }
    } else {
        cliInfos.resize(0);
    }
    out["cliInfos"] = cliInfos;

    Json::Value timerTasksInfo = Json::objectValue;
    this->timerTasksInfo.JsonMarshal(timerTasksInfo);
    out["timerTasksInfo"] = timerTasksInfo;

    return true;
}

bool LocalInfo::JsonUnmarshal(Json::Value in) {
    if (in["serverInfos"].isArray()) {
        Json::Value serverInfos = in["serverInfos"];
        for (auto iter:serverInfos) {
            ServerInfo item;
            if (item.JsonUnmarshal(iter)) {
                this->serverInfos.push_back(item);
            }
        }
    }

    if (in["cliInfos"].isArray()) {
        Json::Value cliInfos = in["cliInfos"];
        for (auto iter:cliInfos) {
            CliInfo item;
            if (item.JsonUnmarshal(iter)) {
                this->cliInfos.push_back(item);
            }
        }
    }

    if (in["timerTasksInfo"].isObject()) {
        Json::Value timerTasksInfo = in["timerTasksInfo"];
        this->timerTasksInfo.JsonUnmarshal(timerTasksInfo);
    }

    return true;
}

bool TimerTasksInfo::JsonMarshal(Json::Value &out) {
    Json::Value names = Json::arrayValue;
    if (!this->names.empty()) {
        for (auto iter:this->names) {
            Json::Value item;
            item["name"] = iter;
            names.append(item);
        }
    } else {
        names.resize(0);
    }
    out["names"] = names;
    return true;
}

bool TimerTasksInfo::JsonUnmarshal(Json::Value in) {
    if (!in["names"].empty()) {
        Json::Value names = in["names"];
        for (auto iter:names) {
            string item = iter.asString();
            this->names.push_back(item);
        }
    }
    return true;
}
