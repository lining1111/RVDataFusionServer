//
// Created by lining on 2/21/22.
//
//#include <string>
//#include <map>
//#include <iostream>
//#include "simpleini/SimpleIni.h"
//
//using namespace std;
//
//int main(int argc, char **argv) {
//
//    if (argc < 2) {
//        return -1;
//    }
//
//    CSimpleIniA ini;
//
//    string filePath = string(argv[1]);
//    ini.LoadFile(filePath.c_str());
//    double repateX;//fix 不变
//    double widthX;//跟路口有关
//    double widthY;//跟路口有关
//    double Xmax;//固定不变
//    double Ymax;//固定不变
//    double gatetx;//跟路口有关
//    double gatety;//跟路口有关
//    double gatex;//跟路口有关
//    double gatey;//跟路口有关
//
//    const char *S_repateX = ini.GetValue("server", "repateX", "");
//    repateX = atof(S_repateX);
//    const char *S_widthX = ini.GetValue("server", "widthX", "");
//    widthX = atof(S_widthX);
//    const char *S_widthY = ini.GetValue("server", "widthY", "");
//    widthY = atof(S_widthY);
//    const char *S_Xmax = ini.GetValue("server", "Xmax", "");
//    Xmax = atof(S_Xmax);
//    const char *S_Ymax = ini.GetValue("server", "Ymax", "");
//    Ymax = atof(S_Ymax);
//    const char *S_gatetx = ini.GetValue("server", "gatetx", "");
//    gatetx = atof(S_gatetx);
//    const char *S_gatety = ini.GetValue("server", "gatety", "");
//    gatety = atof(S_gatety);
//    const char *S_gatex = ini.GetValue("server", "gatex", "");
//    gatex = atof(S_gatex);
//    const char *S_gatey = ini.GetValue("server", "gatey", "");
//    gatey = atof(S_gatey);
//
//    ini.SetValue("server", "getey", "505");
//
//
//}

#include <iostream>
#include <string>
#include "Poco/Data/Session.h"
#include "Poco/Data/SQLite/Connector.h"
using namespace Poco::Data;
using Poco::Data::Keywords::now;
using Poco::Data::Keywords::use;
using Poco::Data::Keywords::bind;
using Poco::Data::Keywords::into;
using Poco::Data::Statement;

using namespace std;
int main()
{
    std::string caseno="CNO1311";
    SQLite::Connector::registerConnector();
    int count = 0;
    Session ses("SQLite","/tmp/hongrui.db");
    ses << "DROP TABLE IF EXISTS Simpsons", now;
    ses << "CREATE TABLE IF NOT EXISTS Simpsons (LastName VARCHAR(30), FirstName VARCHAR, Address VARCHAR, Age INTEGER(3))", now;
    ses << "SELECT COUNT(*) FROM Simpsons ", into(count), now;
    std::cout << "People in Simpsons " << count;
    SQLite::Connector::unregisterConnector();
    return 0;
}