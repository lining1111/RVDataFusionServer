//
// Created by lining on 2023/2/23.
//

#ifndef EOCCOM_H
#define EOCCOM_H

#include "Queue.h"
#include "net/tlsClient/TlsClient.h"
#include "EOCJSON.h"
/**
 * EOC 通信类
 */

class EOCCom : public TlsClient {

private:
    bool businessStart = false;
    Queue<std::string> pkgs;//分包
    //一些通信包
    std::future<int> future_getPkgs;
    std::future<int> future_proPkgs;
    std::future<int> future_interval;
    bool isLogIn = false;
    time_t last_login_t = time(nullptr);//上次登录时间
    time_t last_send_heart_t = time(nullptr);//上次发送心态时间
    time_t last_send_net_state_t = time(nullptr);//上次发送外围状态时间
    time_t last_get_config_t = time(nullptr);//上次获取配置时间

public:
    EOCCom(string ip, int port, string caPath);

    ~EOCCom();

    int Open();

    int Run();

    int Close();

private:
    static int getPkgs(void *p);

    static int proPkgs(void *p);

    static int intervalPro(void *p);

private:
    /**
     * 发生登录eoc指令
     * @return 0：成功 -1：失败
     */
    int sendLogin();

};


#endif //EOCCOM_H
