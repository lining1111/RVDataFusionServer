//
// Created by lining on 11/4/22.
//

#ifndef _EOC_H
#define _EOC_H

#include <string>

using namespace std;

string GetEocCommonVersion();

int StartEocCommon();

int StartEocCommon();

//设置ntp服务地址
int SetNtpServer(string serverIp);

#endif //_EOC_H
