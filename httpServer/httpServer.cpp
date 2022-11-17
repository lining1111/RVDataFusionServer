//
// Created by lining on 10/10/22.
//

#include "httpServer.h"
#include <httplib.h>


#ifdef x86

#include <jsoncpp/json/json.h>

#else

#include <json/json.h>

#endif

httplib::Server svr;
static Local *mLocal = nullptr;

void GetServerInfoCB(const httplib::Request &req, httplib::Response &resp) {
    resp.set_header("Content-Type", "plain/text");
    resp.status = 200;

    FusionServerInfo fusionServerInfo;
    if (mLocal->server->vectorClient.empty()) {
        fusionServerInfo.clientNum = 0;
    } else {
        fusionServerInfo.clientNum = mLocal->server->vectorClient.size();
        for (int i = 0; i < mLocal->server->vectorClient.size(); i++) {
            auto iter = mLocal->server->vectorClient.at(i);
            FusionClientInfo item;
            item.ip = string(inet_ntoa(iter->clientAddr.sin_addr));
            item.direction = iter->direction;
            fusionServerInfo.fusionClientInfos.push_back(item);
        }
    }

    string content;
    JsonMarshalFusionServerInfo(fusionServerInfo, content);

    resp.set_content(content, "plain/text");
}


void GetLocalConnectInfoCB(const httplib::Request &req, httplib::Response &resp) {
    resp.set_header("Content-Type", "plain/text");
    resp.status = 200;

    LocalConnectInfo localConnectInfo;

    localConnectInfo.isRun = mLocal->isRun;
    localConnectInfo.isFusionServerRun = mLocal->server->isRun;
    localConnectInfo.isMerge = mLocal->server->isMerge;
    localConnectInfo.isMultiviewServerRun = mLocal->multiviewServer->isRun;
    localConnectInfo.isClientRun = mLocal->client->isRun;


    string content;
    JsonMarshalLocalConnectInfo(localConnectInfo, content);

    resp.set_content(content, "plain/text");
}

void MThread(void *p, int port) {
    httplib::Server *svr = (httplib::Server *) p;
    svr->listen("0.0.0.0", port);
}


int HttpServerInit(int port, Local *local) {
    mLocal = local;

    svr.Get("/getServerInfo", GetServerInfoCB);
    svr.Get("/getLocalConnectInfo", GetLocalConnectInfoCB);

//    svr.listen("0.0.0.0", port);
    thread mThread = thread(MThread, &svr, port);
    mThread.detach();

}

int JsonMarshalFusionServerInfo(FusionServerInfo fusionServerInfo, string &out) {
    Json::FastWriter fastWriter;
    Json::Value root;

    root["clientNum"] = fusionServerInfo.clientNum;
    if (fusionServerInfo.clientNum <= 0) {
        root["fusionClientInfos"].resize(0);
    } else {
        Json::Value arrayFusionClientInfos = Json::arrayValue;
        for (int i = 0; i < fusionServerInfo.fusionClientInfos.size(); i++) {
            auto iter = fusionServerInfo.fusionClientInfos.at(i);
            Json::Value item;
            item["ip"] = iter.ip;
            item["direction"] = iter.direction;
            arrayFusionClientInfos.append(item);
        }
        root["fusionClientInfos"] = arrayFusionClientInfos;
    }

    out = fastWriter.write(root);
    return 0;
}

int JsonMarshalLocalConnectInfo(LocalConnectInfo localConnectInfo, string &out) {
    Json::FastWriter fastWriter;
    Json::Value root;

    root["isRun"] = localConnectInfo.isRun;
    root["isFusionServerRun"] = localConnectInfo.isFusionServerRun;
    root["isMerge"] = localConnectInfo.isMerge;
    root["isMultiviewServerRun"] = localConnectInfo.isMultiviewServerRun;
    root["isClientRun"] = localConnectInfo.isClientRun;

    out = fastWriter.write(root);
    return 0;
}
