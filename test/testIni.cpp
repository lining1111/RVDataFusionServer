//
// Created by lining on 2/21/22.
//
#include <string>
#include <map>
#include <iostream>
#include "ParseFlag.h"
#include "simpleini/SimpleIni.h"

using namespace std;

int main(int argc, char **argv) {
    map<string, string> use;
    use["-path"] = "设置ini路径";
    ParseFlag *parseFlag = new ParseFlag(use);

    parseFlag->Parse(argc, argv);

    string iniPath = "./config.ini";
    auto iter = parseFlag->useAgeSet.find("-path");
    if (iter != parseFlag->useAgeSet.end()) {
        iniPath = iter->second;
    }

    FILE *fp = fopen(iniPath.c_str(), "r+");
    if (fp == nullptr) {
        cout << "can not open file:" + iniPath << endl;
        return -1;
    }

    CSimpleIniA ini;
    ini.LoadFile(fp);

    double repateX;//fix 不变
    double widthX;//跟路口有关
    double widthY;//跟路口有关
    double Xmax;//固定不变
    double Ymax;//固定不变
    double gatetx;//跟路口有关
    double gatety;//跟路口有关
    double gatex;//跟路口有关
    double gatey;//跟路口有关

    const char *S_repateX = ini.GetValue("server", "repateX", "");
    repateX = atof(S_repateX);
    const char *S_widthX = ini.GetValue("server", "widthX", "");
    widthX = atof(S_widthX);
    const char *S_widthY = ini.GetValue("server", "widthY", "");
    widthY = atof(S_widthY);
    const char *S_Xmax = ini.GetValue("server", "Xmax", "");
    Xmax = atof(S_Xmax);
    const char *S_Ymax = ini.GetValue("server", "Ymax", "");
    Ymax = atof(S_Ymax);
    const char *S_gatetx = ini.GetValue("server", "gatetx", "");
    gatetx = atof(S_gatetx);
    const char *S_gatety = ini.GetValue("server", "gatety", "");
    gatety = atof(S_gatety);
    const char *S_gatex = ini.GetValue("server", "gatex", "");
    gatex = atof(S_gatex);
    const char *S_gatey = ini.GetValue("server", "gatey", "");
    gatey = atof(S_gatey);

    ini.SetValue("server", "getey", "505");
    ini.SaveFile(iniPath.c_str());


    fclose(fp);
}