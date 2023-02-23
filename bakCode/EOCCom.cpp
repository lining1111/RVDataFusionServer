//
// Created by lining on 2023/2/23.
//

#include "EOCCom.h"
#include "db/DBTable.h"
#include <glog/logging.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>

int EOCCom::Open() {
    int ret = 0;
    ret = TlsClient::Open();
    if (ret != 0) {
        LOG(ERROR) << "eoc server connect fail";
    }
    return ret;
}

int EOCCom::Run() {
    int ret = 0;
    ret = TlsClient::Run();
    businessStart = true;
    future_getPkgs = std::async(std::launch::async, getPkgs, this);

    future_proPkgs = std::async(std::launch::async, proPkgs, this);

    future_interval = std::async(std::launch::async, intervalPro, this);
}

int EOCCom::Close() {
    int ret = 0;
    ret = TlsClient::Close();
    if (businessStart) {
        try {
            future_getPkgs.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }

        try {
            future_proPkgs.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }

        try {
            future_interval.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    businessStart = false;
}

int EOCCom::getPkgs(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    char *buf = new char[4096];
    int index = 0;
    memset(buf, 0, 4096);
    while (local->isLive.load()) {
        usleep(1000);
        while (local->rb->GetReadLen() > 0) {
            char value = 0x00;
            if (local->rb->Read(&value, 1) > 0) {
                if (value == '*') {
                    //得到分包尾部
                    std::string pkg = std::string(buf);
                    local->pkgs.push(pkg);
                    index = 0;
                    memset(buf, 0, 4096);
                } else {
                    buf[index] = value;
                    index++;
                }
            }
        }
    }
    delete[] buf;
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}

int EOCCom::proPkgs(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    while (local->isLive.load()) {
        usleep(1000);
        std::string pkg;
        if (local->pkgs.pop(pkg)) {
            //接收到一包数据
            LOG(INFO) << "eoc recv json:" << pkg;


        }
    }
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}


/*取板卡本地ip地址和n2n地址 */
int getipaddr(char *ethip, char *n2nip) {
    int ret = 0;
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *pifAddrStruct = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    if (ifAddrStruct != NULL) {
        pifAddrStruct = ifAddrStruct;
    }

    while (ifAddrStruct != NULL) {
        if (ifAddrStruct->ifa_addr == NULL) {
            ifAddrStruct = ifAddrStruct->ifa_next;
            LOG(ERROR) << "addr null";
            continue;
        }

        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0) {
                LOG(INFO) << "eth0 IP Address:" << addressBuffer;
                sprintf(ethip, "%s", addressBuffer);
            } else if (strcmp(ifAddrStruct->ifa_name, "n2n0") == 0) {
                LOG(INFO) << "n2n0 IP Address:" << addressBuffer;
                sprintf(n2nip, "%s", addressBuffer);
            }
        }

        ifAddrStruct = ifAddrStruct->ifa_next;
    }

    if (pifAddrStruct != NULL) {
        freeifaddrs(pifAddrStruct);
    }

    return ret;
}

/*
 * 函数功能：产生uuid
 * 参数：无
 * 返回值：uuid的string
 * */
static std::string random_uuid() {
    char buf[37] = {0};
    struct timeval tmp;
    const char *c = "89ab";
    char *p = buf;
    unsigned int n, b;
    gettimeofday(&tmp, NULL);
    srand(tmp.tv_usec);

    for (n = 0; n < 16; ++n) {
        b = rand() % 65536;
        switch (n) {
            case 6:
                sprintf(p, "4%x", b % 15);
                break;
            case 8:
                sprintf(p, "%c%x", c[rand() % strlen(c)], b % 15);
                break;
            default:
                sprintf(p, "%02x", b);
                break;
        }
        p += 2;
        switch (n) {
            case 3:
            case 5:
            case 7:
            case 9:
                *p++ = '-';
                break;
        }
    }
    *p = 0;
    return string(buf);
}

#define COMVersion "V1.0.0"

static int getS100(S100 &data) {
    data.head.Guid = random_uuid();
    data.head.Code = "MCCS100";
    data.head.Version = COMVersion;
    return 0;
}


static int getS101(S101 &data) {
    int ret = 0;

    //获取设备sn
    ret = db_factory_get_uname(data.Data.EquipNumber);
    if (ret != 0) {
        LOG(ERROR) << "get uname err";
        return -1;
    }
    //获取ip信息 ip[n2n]
    char ethip[32] = {0};
    char n2nip[32] = {0};
    char ipmsg[64] = {0};
    memset(ethip, 0, 32);
    memset(n2nip, 0, 32);
    memset(ipmsg, 0, 64);
    getipaddr(ethip, n2nip);
    sprintf(ipmsg, "%s[%s]", ethip, n2nip);
    data.Data.EquipIp = std::string(ipmsg);
    //获取dataVersion
    getVersion(data.Data.DataVersion);

    data.head.Guid = random_uuid();
    data.head.Code = "MCCS101";
    data.head.Version = COMVersion;
    data.Data.EquipType = "XX";
    data.Data.SoftVersion = "V1.0.0";

    return 0;
}

static int getS104(S104 &data) {
    int ret = 0;

    //获取设备sn
    ret = db_factory_get_uname(data.Data.MainboardGuid);
    if (ret != 0) {
        LOG(ERROR) << "get uname err";
        return -1;
    }

    data.head.Guid = random_uuid();
    data.head.Code = "MCCS104";
    data.head.Version = COMVersion;
    return 0;
}

int sendNetTotal = 0;
int sendNetSuccess = 0;

static int getS105(S105 &data) {
    data.head.Guid = random_uuid();
    data.head.Code = "MCCS104";
    data.head.Version = COMVersion;

    data.Data.Total = sendNetTotal;
    data.Data.Success = sendNetSuccess;

    return 0;
}


int EOCCom::intervalPro(void *p) {
    if (p == nullptr) {
        return -1;
    }
    EOCCom *local = (EOCCom *) p;
    LOG(INFO) << __FUNCTION__ << "run";
    while (local->isLive.load()) {
        sleep(10);
        time_t now = time(NULL);
        if (!local->isLogIn) {
            //未登录的话，一直申请登录
            if (now - local->last_login_t > 10) {
                local->last_login_t = now;

                S101 s101;
                int ret = getS101(s101);
                if (ret != 0) {
                    LOG(ERROR) << "get s101 fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s101.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());

                if (len < 0) {
                    LOG(ERROR) << "s101_login_send err, return:" << len;
                } else {
                    LOG(INFO) << "s101_login_send ok";
                }

                local->last_send_heart_t = now - 20;
            }
        } else {
            //登录后，每隔30s上传1次心跳，如果上次获取配置的时间大于180s且获取配置失败的话，主动发送获取配置命令，每300s上传一次外围状态
            //同时处理一些升级下载任务

            //获取配置
            std::string version;
            getVersion(version);
            if (version.empty()) {
                if (now - local->last_get_config_t > 180) {
                    local->last_get_config_t = now;
                    //主动发起一次获取配置的请求
                    S104 s104;
                    int ret = getS104(s104);
                    if (ret != 0) {
                        LOG(ERROR) << "get s104 fail";
                        continue;
                    }

                    //组json
                    Json::FastWriter fastWriter;
                    Json::Value out;
                    s104.JsonMarshal(out);
                    std::string body;
                    body = fastWriter.write(out);
                    body.push_back('*');
                    int len = local->Write(body.data(), body.size());

                    if (len < 0) {
                        LOG(ERROR) << "s104 send err, return:" << len;
                    } else {
                        LOG(INFO) << "s104 send ok";
                    }
                }
            }

            //心跳
            if (now - local->last_send_heart_t > 30) {
                local->last_send_heart_t = now;
                S100 s100;
                int ret = getS100(s100);
                if (ret != 0) {
                    LOG(ERROR) << "get s100 fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s100.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());

                if (len < 0) {
                    LOG(ERROR) << "s100 send err, return:" << len;
                } else {
                    LOG(INFO) << "s100 send ok";
                }
            }

            //外网状态上传
            if (now - local->last_send_net_state_t > 300) {
                local->last_send_net_state_t = now;

                S105 s105;
                int ret = getS105(s105);
                if (ret != 0) {
                    LOG(ERROR) << "get s100 fail";
                    continue;
                }

                //组json
                Json::FastWriter fastWriter;
                Json::Value out;
                s105.JsonMarshal(out);
                std::string body;
                body = fastWriter.write(out);
                body.push_back('*');
                int len = local->Write(body.data(), body.size());
                sendNetTotal++;

                if (len < 0) {
                    LOG(ERROR) << "s100 send err, return:" << len;
                } else {
                    LOG(INFO) << "s100 send ok";
                    if (sendNetTotal == 0) {
                        sendNetSuccess = 0;
                    } else {
                        sendNetSuccess++;
                    }
                }
            }
            //升级、下载TODO
            //处理下载消息
        }
    }
    LOG(INFO) << __FUNCTION__ << "exit";
    return 0;
}

int EOCCom::sendLogin() {


    return 0;
}
