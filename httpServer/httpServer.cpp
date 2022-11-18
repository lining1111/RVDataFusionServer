//
// Created by lining on 10/10/22.
//

#include "httpServer.h"
#include <httplib.h>

httplib::Server svr;
static Local *mLocal = nullptr;

void GetLocalInfoCB(const httplib::Request &req, httplib::Response &resp) {
    resp.set_header("Content-Type", "plain/text");
    resp.status = 200;

    LocalInfo localInfo;

    for (auto iter:mLocal->serverList) {
        ServerInfo item;
        item.ip = "localhost";
        item.port = iter->port;
        for (auto iter1:iter->vectorClient) {
            ServerClientInfo item1;
            item1.ip = string(inet_ntoa(iter1->clientAddr.sin_addr));
            item1.direction = iter1->direction;
            item.clients.push_back(item1);
        }
        localInfo.serverInfos.push_back(item);
    }

    for (auto iter:mLocal->clientList) {
        CliInfo item;
        item.serverIp = iter->server_ip;
        item.serverPort = iter->server_port;
        localInfo.cliInfos.push_back(item);
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
    svr->listen("0.0.0.0", port);
}


int HttpServerInit(int port, Local *local) {
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

    return true;
}
