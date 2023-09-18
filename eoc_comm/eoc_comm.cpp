/*
 * gps_collection.cpp
 *
 *  Created on: 2021年11月20日
 *      Author: pengchao
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ifaddrs.h>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <iostream>
#include "cJSON.h"
#include "eoc_comm.hpp"
#include "sqlite_eoc_db.h"
#include "configure_eoc_init.h"
#include "dns_server.h"
#include "md5.h"
#include "MySystemCmd.hpp"

#include "logger.h"

using namespace std;


extern EOC_Base_Set g_eoc_base_set;    //基础配置


//#define LICENSEFILE "zkj.lic"
#define UPDATEFILE  "zkj_update.tgz"
#define UPDATEUNZIPFILE HOME_PATH "/zkj_update_tmp"  // 升级包解压路径
#define INSTALLSH "update.sh"  // 升级脚本

static EOCCloudData eoc_comm_data;

class LockGuard {
public:
    LockGuard(pthread_mutex_t *lock) :
            lock_(lock) {
        pthread_mutex_lock(lock_);
    }

    virtual ~LockGuard() {
        pthread_mutex_unlock(lock_);
    }

private:
    pthread_mutex_t *lock_;
};


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
            DBG("addr NULL!");
            continue;
        }

        if (ifAddrStruct->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            if (strcmp(ifAddrStruct->ifa_name, "eth0") == 0) {
                DBG("eth0 IP Address %s", addressBuffer);
                sprintf(ethip, "%s", addressBuffer);
            } else if (strcmp(ifAddrStruct->ifa_name, "n2n0") == 0) {
                DBG("n2n0 IP Address %s", addressBuffer);
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
 * 将字符串分割
 * 按照指定的分割字符组对字符串进行分割
 * 如：
 * 对"adf*sdsdff*sdgsfgsdad"分割将得到adf、sdsdff、sdgsfgsdad
 *
 * 参数：
 * str：待分割字符串
 * delimiters：作为分割使用的字符组
 * sv：分割出来的字符串
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int splitstr2vector(const char *str, const char *delimiters, std::vector<std::string> &sv) {
    sv.clear();

    char *ptr = NULL;
    char *p = NULL;
    char *str_tmp = (char *) malloc(strlen(str) + 1);
    if (str_tmp == NULL) {
        ERR("%s:malloc() Error\n", __FUNCTION__);
        return -1;
    }

    sprintf(str_tmp, "%s", str);
    ptr = strtok_r(str_tmp, delimiters, &p);
    while (ptr != NULL) {
        sv.push_back(ptr);
        ptr = strtok_r(NULL, delimiters, &p);
    }
    free(str_tmp);

    return 0;
}

/*
 * 函数功能：产生uuid
 * 参数：无
 * 返回值：uuid的string
 * */
static string random_uuid() {
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

/*
 * 函数功能：获取当前时间
 * */
static string get_current_time() {
    time_t now;
    struct tm timenow;
    char p[50] = {0};

    time(&now);
    localtime_r(&now, &timenow);
    sprintf(p, "%d%02d%02d%02d%02d%02d",
            1900 + timenow.tm_year,
            1 + timenow.tm_mon,
            timenow.tm_mday,
            timenow.tm_hour,
            timenow.tm_min,
            timenow.tm_sec);

    return string(p);
}

static int compute_file_md5(const char *file_path, const char *md5_check) {
    int fd;
    int ret;
    unsigned char data[1024];
    unsigned char md5_value[16];
    char md5_tmp[64] = {0};
    MD5_CTX md5;

    fd = open(file_path, O_RDONLY);
    if (-1 == fd) {
        ERR("%s:open file[%s] Err", __FUNCTION__, file_path);
        return -1;
    }
    MD5Init(&md5);
    while (1) {
        ret = read(fd, data, 1024);
        if (-1 == ret) {
            ERR("%s:read file[%s] Err", __FUNCTION__, file_path);
            return -1;
        }
        MD5Update(&md5, data, ret);
        if (0 == ret || ret < 1024) {
            break;
        }
    }
    close(fd);
    MD5Final(&md5, md5_value);

    for (int i = 0; i < 16; i++) {
        snprintf(md5_tmp + i * 2, 2 + 1, "%02x", md5_value[i]);
    }

    if (strcmp(md5_check, md5_tmp) == 0) {
        return 0;
    } else {
        INFO("%s md5check failed:file_md5=%s,check_md5=%s", __FUNCTION__, md5_tmp, md5_check);
    }

    return -1;
}

static ssize_t writen(int fd, const void *vptr, size_t n) {
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = (const char *) vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;        /* and call write() again */
            else
                return (-1);            /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

static size_t download_file_callback(void *buffer, size_t size, size_t nmemb, void *userp) {
    static int count = 0;
    if (count++ % 20 == 0) {
        printf(".");
        fflush(stdout);
    }
    return writen(*(int *) userp, buffer, nmemb * size);
}

/*
 * 下载并保存到本地（程序运行当前路径下）
 *
 * 返回值：
 * 0：成功
 * -1：下载文件不存在
 * -2：下载超时
 * -3：本地剩余空间不足
 * */
static int eoc_download_file(const char *url, int timeout_ms, const char *file_name,
                             int file_size, const char *file_md5) {
    DBG("%s:url=%s,timeout_ms=%d,file_name=%s,file_size=%d,file_md5=%s", __FUNCTION__,
        url, timeout_ms, file_name, file_size, file_md5);
    int ret = 0;
    CURL *curl;
    CURLcode res;
    vector<char> file_buff;
    struct curl_slist *headerlist = NULL;

    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd <= 0) {
        ERR("%s:create file:%s Err", __FUNCTION__, file_name);
        return -3;
    }
    std::string ipurl;
    std::string host, port, ipaddr;
    search_DNS_ipurl(url, ipurl, 0, host, port, ipaddr);
    host = "Host:" + host;
    port = "Port:" + port;
    headerlist = curl_slist_append(headerlist, "Expect:");
    //headerlist = curl_slist_append(headerlist, "Accept: application/json");
    headerlist = curl_slist_append(headerlist, host.c_str());
    headerlist = curl_slist_append(headerlist, port.c_str());

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);            //接收数据时超时设置，如果10秒内数据未接收完，直接退出
        //curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);                       //查找次数，防止查找太深
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);      //连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
        curl_easy_setopt(curl, CURLOPT_URL, (char *) ipurl.c_str());//请求的URL
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_file_callback);     //数据回来后的回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fd);              //回调函数里面用到的参数
        //Todo:302重定向问题
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);    // 设置重定向的最大次数
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);    // 设置301、302跳转跟随location
//      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);                         //可视化调试

        res = curl_easy_perform(curl);
        long http_response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_response_code);
        DBG("%s:res=%d,response_code=%ld", __FUNCTION__, res, http_response_code);
        if (res == CURLE_OK && http_response_code == 200) {
            //请求成功
            DBG("download_file[%s] to %s succeed", url, file_name);
            ret = 0;
            if (compute_file_md5(file_name, file_md5) != 0) {
                ERR("%s:%s MD5校验失败", __FUNCTION__, file_name);
                ret = -4;
            }
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -2;
        } else if (res == CURLE_WRITE_ERROR) {
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -3;
        } else {
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -1;
        }
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(headerlist);
    close(fd);
    return ret;
}

static void *eoc_download_thread(void *argv) {
    DBG("%s:Enter.argv:%s", __FUNCTION__, (char *) argv);
    int ret = 0;
    char *download_url = (char *) argv;
    char update_file_name[256] = {0};
    size_t tmp_i = 0;
    for (tmp_i = 0; tmp_i < eoc_comm_data.downloads_msg.size(); tmp_i++) {
        DBG("[%ld]url:%s", tmp_i, eoc_comm_data.downloads_msg[tmp_i].download_url.c_str());
        if (strcmp(eoc_comm_data.downloads_msg[tmp_i].download_url.c_str(), download_url) == 0) {
            //找到下载链接信息
            if (eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size() == 0) {
                //主控板升级下载文件
                snprintf(update_file_name, 256, "%s", UPDATEFILE);
            } else {
                if (eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[0].dev_type == EOC_UPGRADE_PARKINGAREA) {
                    //主控板升级下载文件
                    snprintf(update_file_name, 256, "%s", UPDATEFILE);
                } else {
                    //相机或矩阵控制器升级下载文件
                    snprintf(update_file_name, 256, "%s_%s",
                             eoc_comm_data.downloads_msg[tmp_i].download_file_md5.c_str(),
                             eoc_comm_data.downloads_msg[tmp_i].download_file_name.c_str());
                }
            }
            ret = eoc_download_file(eoc_comm_data.downloads_msg[tmp_i].download_url.c_str(),
                                    8 * 60 * 1000,
                                    update_file_name,
                                    eoc_comm_data.downloads_msg[tmp_i].download_file_size,
                                    eoc_comm_data.downloads_msg[tmp_i].download_file_md5.c_str());
            if (ret == 0) {
                DBG("%s:下载文件完成", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_FINISHED;
            } else if (ret == -1) {
                ERR("%s:下载文件不存在", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_FILE_NOT_EXIST;
            } else if (ret == -2) {
                ERR("%s:下载超时", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_TIMEOUT;
            } else if (ret == -3) {
                ERR("%s:本地剩余空间不足", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_SPACE_NOT_ENOUGH;
            } else if (ret == -4) {
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_MD5_FAILD;
            }
            break;
        }
    }

    if (tmp_i == eoc_comm_data.downloads_msg.size()) {
        DBG("serch download_url err");
    }

    DBG("%s:Exit.", __FUNCTION__);
    return NULL;
}


bool is_login = false;
int heart_flag = 0;
static string db_conf_version;
static string eoc_rec_data; //接收到的数据

#define EOC_COMM_RECEIVE_100 "MCCR100"    //心跳
#define EOC_COMM_SEND_100    "MCCS100"
#define EOC_COMM_RECEIVE_101 "MCCR101"    //登录
#define EOC_COMM_SEND_101    "MCCS101"
#define EOC_COMM_RECEIVE_102 "MCCR102"    //配置下发
#define EOC_COMM_SEND_102    "MCCS102"
#define EOC_COMM_RECEIVE_104 "MCCR104"    //配置获取
#define EOC_COMM_SEND_104    "MCCS104"
#define EOC_COMM_RECEIVE_105 "MCCR105"    //外网状态上传
#define EOC_COMM_SEND_105    "MCCS105"
#define EOC_COMM_RECEIVE_106 "MCCR106"    //软件下载
#define EOC_COMM_SEND_106    "MCCS106"
#define EOC_COMM_RECEIVE_107 "MCCR107"    //软件升级
#define EOC_COMM_SEND_107    "MCCS107"
#define EOC_COMM_SEND_103    "MCCS103"


//添加下载任务
//返回值：0添加成功；-1添加失败; 1正在升级退出
int add_eoc_download_event(EocDownloadsMsg &data, EocUpgradeDev &dev_data) {
    int ret = 0;

    size_t tmp_i = 0;
    size_t tmp_j = 0;
    for (tmp_i = 0; tmp_i < eoc_comm_data.downloads_msg.size(); tmp_i++) {
        if (data.download_url == eoc_comm_data.downloads_msg[tmp_i].download_url) {
            //下载链接已存在
            DBG("%s:download_thread is running, add ip %s", __FUNCTION__, dev_data.dev_ip.c_str());
            if (dev_data.dev_type == EOC_UPGRADE_PARKINGAREA) {
                DBG("雷视机升级下载已添加");
                return 1;
            }
            for (tmp_j = 0; tmp_j < eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size(); tmp_j++) {
                if ((eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[tmp_j].dev_ip == dev_data.dev_ip)
                    && (eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[tmp_j].dev_guid == dev_data.dev_guid)) {
                    DBG("固件下载已添加");
                    return 1;
                }
            }
            if (tmp_j == eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size()) {
                DBG("固件下载添加");
                eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.push_back(dev_data);
            }
            //    eoc_comm_data.downloads_msg[tmp_i].comm_guid = data.comm_guid;
            ret = 0;
            break;
        }
    }
    if (tmp_i == eoc_comm_data.downloads_msg.size()) {
        //下载链接不存在，添加下载
        data.download_status = DOWNLOAD_IDLE;
        data.upgrade_dev.clear();
        data.upgrade_dev.push_back(dev_data);
        eoc_comm_data.downloads_msg.push_back(data);
        ret = pthread_create(&eoc_comm_data.downloads_msg[tmp_i].download_thread_id, NULL,
                             eoc_download_thread, (void *) eoc_comm_data.downloads_msg[tmp_i].download_url.c_str());
        if (ret != 0) {
            ERR("%s:create download_thread Err", __FUNCTION__);
            return -1;
        }
    }

    return 0;
}

//添加升级任务
//返回值：0添加成功；-1添加失败; 1升级任务已存在
int add_eoc_upgrade_event(EocUpgradeDev &data) {
    int ret = 0;
    //判断此guid设备是否在升级队列，不在添加
    size_t tmp_i = 0;
    for (tmp_i = 0; tmp_i < eoc_comm_data.upgrade_msg.size(); tmp_i++) {
        if (eoc_comm_data.upgrade_msg[tmp_i].dev_guid == data.dev_guid) {
            DBG("升级guid已存在, ip:%s", data.dev_ip.c_str());
            return 1;
        }
    }
    if (tmp_i == eoc_comm_data.upgrade_msg.size()) {
        EocUpgradeDev rcv_upgrade_dev;
        rcv_upgrade_dev.dev_guid = data.dev_guid;
        rcv_upgrade_dev.dev_ip = data.dev_ip;
        rcv_upgrade_dev.dev_type = data.dev_type;
        rcv_upgrade_dev.start_upgrade_time = time(NULL);
        rcv_upgrade_dev.sw_version = data.sw_version;
        rcv_upgrade_dev.hw_version = data.hw_version;
        rcv_upgrade_dev.dev_model = data.dev_model;
        rcv_upgrade_dev.upgrade_mode = data.upgrade_mode;
        rcv_upgrade_dev.upgrade_time = "";
        rcv_upgrade_dev.comm_guid = data.comm_guid;
        rcv_upgrade_dev.up_status = data.up_status;
        rcv_upgrade_dev.up_progress = data.up_progress;
        eoc_comm_data.upgrade_msg.push_back(rcv_upgrade_dev);
        DBG("升级任务无相同guid, push_back(), ip:%s", data.dev_ip.c_str());
    }

    return 0;
}

//添加系统重启任务
static int add_reboot_task() {
    unsigned int tmp_i = 0;
    if ((eoc_comm_data.task.size() == 0) && (eoc_comm_data.downloads_msg.size() == 0) &&
        (eoc_comm_data.upgrade_msg.size() == 0)) {
        DBG("System is going to reboot!");
        sleep(5);
        _exit(0);
    }
    for (tmp_i = 0; tmp_i < eoc_comm_data.task.size(); tmp_i++) {
        if (eoc_comm_data.task[tmp_i] == SYS_REBOOT) {
            DBG("进程重启任务已存在");
            break;
        }
    }
    if (tmp_i == eoc_comm_data.task.size()) {
        eoc_comm_data.task.push_back(SYS_REBOOT);
    }
    return 0;
}


static int get_json_int(cJSON *j_data, string str) {
    if (cJSON_GetObjectItem(j_data, str.c_str()) == NULL) {
        ERR("json get int %s err", str.c_str());
        return 0;
    } else {
        return cJSON_GetObjectItem(j_data, str.c_str())->valueint;
    }
}

static double get_json_double(cJSON *j_data, string str) {
    if (cJSON_GetObjectItem(j_data, str.c_str()) == NULL) {
        ERR("json get int %s err", str.c_str());
        return 0;
    } else {
        return cJSON_GetObjectItem(j_data, str.c_str())->valuedouble;
    }
}

static string get_json_string(cJSON *j_data, string str) {
    string get_str;
    if (cJSON_GetObjectItem(j_data, str.c_str()) == NULL) {
        ERR("json get string %s err", str.c_str());
        get_str = "";
        return get_str;
    } else {
        if (cJSON_GetObjectItem(j_data, str.c_str())->valuestring != NULL) {
            get_str = cJSON_GetObjectItem(j_data, str.c_str())->valuestring;
        } else {
            DBG("%s = null", str.c_str());
            get_str = "";
        }
        return get_str;
    }
}

/*
 * 向eoc发送心跳
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s100_heart_send(tcp_client_t *client) {
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char *json_str = NULL;
    string send_str;

    //打包返回信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_100);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_100);

    json_str = cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

/*
 * 处理eoc返回的心跳信息
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int r100_heart_deal(cJSON *json) {
    int ret = 0;
    char code[16] = {0};
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    sprintf(code, "%s", get_json_string(data_json, "Code").c_str());

    if (strcmp(code, EOC_COMM_RECEIVE_100) == 0) {
        heart_flag = 0;
        DBG("receive EARLER100");
    } else {
        ERR("rec 100 Data-Code err : %s", code);
    }

    return 0;
}

#include "version.h"

/*
 * 登录eoc
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s101_login_send(tcp_client_t *client) {
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char *json_str = NULL;
    string equip_num;
    char ethip[32] = {0};
    char n2nip[32] = {0};
    char ipmsg[64] = {0};
    string send_str;

    ret = db_factory_get_uname(equip_num);
    if (ret != 0) {
        ERR("%s:get_factory_uname() Err", __FUNCTION__);
        return -1;
    }
//    equip_num = "aab65d77-8242-4f31-b088-46eb038663d9";

    getipaddr(ethip, n2nip);
    sprintf(ipmsg, "%s[%s]", ethip, n2nip);

    string db_data_version;
    db_version_get(db_data_version);

    //打包发送信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        DBG("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_101);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_101);
    cJSON_AddStringToObject(data, "EquipNumber", equip_num.c_str());
    cJSON_AddStringToObject(data, "EquipIp", ipmsg);
    cJSON_AddStringToObject(data, "EquipType", "XX");
    cJSON_AddStringToObject(data, "SoftVersion", VERSION_BUILD_TIME);
    cJSON_AddStringToObject(data, "DataVersion", db_data_version.c_str());

    json_str = cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";
    DBG("%s:%s", __FUNCTION__, send_str.c_str());
    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);

    return 0;
}

static int r101_login_deal(cJSON *json) {
    int ret = 0;

    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    if (get_json_int(data_json, "State") == 1) {
        DBG("101登录完成");
        is_login = true;
        ret = 0;
    } else {
        ERR("101登录失败：%s", get_json_string(data_json, "Message").c_str());//读取错误信息
        ret = 0;
    }

    return ret;
}

/*
 * 向eoc发送配置结果
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s102_config_download_send(tcp_client_t *client, const char *comm_guid, int state, const char *msg) {
    if (strlen(comm_guid) <= 0) {
        DBG("comm_guid null");
        return 0;
    }

    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char *json_str = NULL;
    string send_str;

    //打包返回信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_102);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_102);
    cJSON_AddNumberToObject(data, "State", state);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str = cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";
    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return 0;
}

/*
 * 更新配置信息版本号
 *
 * 0：成功
 * -1：失败
 * */
static int update_conf_version(const char *version) {
    //更新数据库版本号
    int ret = 0;
    DB_Conf_Data_Version data;
    time_t now;
    struct tm timenow;
    time(&now);
    localtime_r(&now, &timenow);
    char time_p[50] = {0};
    sprintf(time_p, "%u-%02u-%02u %02u:%02u:%02u",
            1900 + timenow.tm_year, 1 + timenow.tm_mon, timenow.tm_mday,
            timenow.tm_hour, timenow.tm_min, timenow.tm_sec);
    DBG("write to database,dbversion = %s, time_p = %s", version, time_p);

    ret = db_version_delete();
    if (ret != 0) {
        ERR("%s:db_delete_version() Err", __FUNCTION__);
        return -1;
    }

    data.Id = 0;
    data.version = version;
    data.time = time_p;

    ret = db_version_insert(data);
    if (ret != 0) {
        ERR("%s:db_insert_version() Err", __FUNCTION__);
        return -1;
    }

    return 0;
}

/*
 * 处理eoc配置下发
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int r102_config_download_deal(tcp_client_t *client, const char *comm_guid, cJSON *json) {
    int ret = 0;
    DB_Base_Set_T base_set_db;
    DB_Intersection_T intersection_db;

    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    if (data_json == NULL) {
        ERR("%s:get object item Data NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item Data NULL");
        return ret;
    }
    base_set_db.Index = get_json_int(data_json, "Index");
    //核心板基础配置
    cJSON *base_set_json = cJSON_GetObjectItem(data_json, "BaseSetting");
    if (base_set_json == NULL) {
        ERR("%s:get object item BaseSetting NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item BaseSetting NULL");
        return ret;
    }
    base_set_db.City = get_json_string(base_set_json, "City");
    base_set_db.IsUploadToPlatform = get_json_int(base_set_json, "IsUploadToPlatform");
    base_set_db.Is4Gmodel = get_json_int(base_set_json, "Is4Gmodel");
    base_set_db.IsIllegalCapture = get_json_int(base_set_json, "IsIllegalCapture");
    base_set_db.Remarks = get_json_string(base_set_json, "Remarks");
    base_set_db.IsPrintIntersectionName = get_json_int(base_set_json, "IsPrintIntersectionName");
    base_set_db.FilesServicePath = get_json_string(base_set_json, "FilesServicePath");
    base_set_db.FilesServicePort = get_json_int(base_set_json, "FilesServicePort");
    base_set_db.MainDNS = get_json_string(base_set_json, "MainDNS");
    base_set_db.AlternateDNS = get_json_string(base_set_json, "AlternateDNS");
    base_set_db.PlatformTcpPath = get_json_string(base_set_json, "PlatformTcpPath");
    base_set_db.PlatformTcpPort = get_json_int(base_set_json, "PlatformTcpPort");
    base_set_db.PlatformHttpPath = get_json_string(base_set_json, "PlatformHttpPath");
    base_set_db.PlatformHttpPort = get_json_int(base_set_json, "PlatformHttpPort");
    base_set_db.SignalMachinePath = get_json_string(base_set_json, "SignalMachinePath");
    base_set_db.SignalMachinePort = get_json_int(base_set_json, "SignalMachinePort");
    base_set_db.IsUseSignalMachine = get_json_int(base_set_json, "IsUseSignalMachine");
    base_set_db.NtpServerPath = get_json_string(base_set_json, "NtpServerPath");
    base_set_db.FusionMainboardIp = get_json_string(base_set_json, "FusionMainboardIp");
    base_set_db.FusionMainboardPort = get_json_int(base_set_json, "FusionMainboardPort");
    base_set_db.IllegalPlatformAddress = get_json_string(base_set_json, "IllegalPlatformAddress");
    db_base_set_delete();
    db_base_set_insert(base_set_db);

    //所属路口信息
    cJSON *intersection_info_json = cJSON_GetObjectItem(data_json, "IntersectionInfo");
    if (intersection_info_json == NULL) {
        ERR("%s:get object item IntersectionInfo NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item IntersectionInfo NULL");
        return ret;
    }
    intersection_db.Guid = get_json_string(intersection_info_json, "Guid");
    intersection_db.Name = get_json_string(intersection_info_json, "Name");
    intersection_db.Type = get_json_int(intersection_info_json, "Type");
    intersection_db.PlatId = get_json_string(intersection_info_json, "PlatId");
    intersection_db.XLength = get_json_double(intersection_info_json, "XLength");
    intersection_db.YLength = get_json_double(intersection_info_json, "YLength");
    intersection_db.LaneNumber = get_json_int(intersection_info_json, "LaneNumber");
    intersection_db.Latitude = get_json_string(intersection_info_json, "Latitude");
    intersection_db.Longitude = get_json_string(intersection_info_json, "Longitude");
    //-----路口参数设置IntersectionBaseSetting
    cJSON *intersect_set_json = cJSON_GetObjectItem(intersection_info_json, "IntersectionBaseSetting");
    if (intersect_set_json == NULL) {
        ERR("%s:get object item IntersectionBaseSetting null", __FUNCTION__);
        intersection_db.FlagEast = 0;
        intersection_db.FlagSouth = 0;
        intersection_db.FlagWest = 0;
        intersection_db.FlagNorth = 0;
        intersection_db.DeltaXEast = 0.0;
        intersection_db.DeltaYEast = 0.0;
        intersection_db.DeltaXSouth = 0.0;
        intersection_db.DeltaYSouth = 0.0;
        intersection_db.DeltaXWest = 0.0;
        intersection_db.DeltaYWest = 0.0;
        intersection_db.DeltaXNorth = 0.0;
        intersection_db.DeltaYNorth = 0.0;
        intersection_db.WidthX = 0.0;
        intersection_db.WidthY = 0.0;
    } else {
        intersection_db.FlagEast = get_json_int(intersect_set_json, "FlagEast");
        intersection_db.FlagSouth = get_json_int(intersect_set_json, "FlagSouth");
        intersection_db.FlagWest = get_json_int(intersect_set_json, "FlagWest");
        intersection_db.FlagNorth = get_json_int(intersect_set_json, "FlagNorth");
        intersection_db.DeltaXEast = get_json_double(intersect_set_json, "DeltaXEast");
        intersection_db.DeltaYEast = get_json_double(intersect_set_json, "DeltaYEast");
        intersection_db.DeltaXSouth = get_json_double(intersect_set_json, "DeltaXSouth");
        intersection_db.DeltaYSouth = get_json_double(intersect_set_json, "DeltaYSouth");
        intersection_db.DeltaXWest = get_json_double(intersect_set_json, "DeltaXWest");
        intersection_db.DeltaYWest = get_json_double(intersect_set_json, "DeltaYWest");
        intersection_db.DeltaXNorth = get_json_double(intersect_set_json, "DeltaXNorth");
        intersection_db.DeltaYNorth = get_json_double(intersect_set_json, "DeltaYNorth");
        intersection_db.WidthX = get_json_double(intersect_set_json, "WidthX");
        intersection_db.WidthY = get_json_double(intersect_set_json, "WidthY");
    }
    db_belong_intersection_delete();
    db_belong_intersection_insert(intersection_db);

    //融合参数
    cJSON *fusion_para_set_json = cJSON_GetObjectItem(data_json, "FusionParaSetting");
    if (fusion_para_set_json == NULL) {
        ERR("%s:get object item FusionParaSetting NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item FusionParaSetting NULL");
        return ret;
    }
    DB_Fusion_Para_Set_T fusion_para_db;
    fusion_para_db.IntersectionAreaPoint1X = get_json_double(fusion_para_set_json, "IntersectionAreaPoint1X");
    fusion_para_db.IntersectionAreaPoint1Y = get_json_double(fusion_para_set_json, "IntersectionAreaPoint1Y");
    fusion_para_db.IntersectionAreaPoint2X = get_json_double(fusion_para_set_json, "IntersectionAreaPoint2X");
    fusion_para_db.IntersectionAreaPoint2Y = get_json_double(fusion_para_set_json, "IntersectionAreaPoint2Y");
    fusion_para_db.IntersectionAreaPoint3X = get_json_double(fusion_para_set_json, "IntersectionAreaPoint3X");
    fusion_para_db.IntersectionAreaPoint3Y = get_json_double(fusion_para_set_json, "IntersectionAreaPoint3Y");
    fusion_para_db.IntersectionAreaPoint4X = get_json_double(fusion_para_set_json, "IntersectionAreaPoint4X");
    fusion_para_db.IntersectionAreaPoint4Y = get_json_double(fusion_para_set_json, "IntersectionAreaPoint4Y");
    db_fusion_para_set_delete();
    db_fusion_para_set_insert(fusion_para_db);

    //关联设备
    cJSON *associated_equip_array_json = cJSON_GetObjectItem(data_json, "AssociatedEquips");
    if (associated_equip_array_json == NULL) {
        ERR("%s:get object item AssociatedEquips NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item AssociatedEquips NULL");
        return ret;
    }
    int associated_equip_array_size = cJSON_GetArraySize(associated_equip_array_json);
    db_associated_equip_delete();
    for (int i = 0; i < associated_equip_array_size; i++) {
        cJSON *associated_equip_json = cJSON_GetArrayItem(associated_equip_array_json, i);
        if (associated_equip_json == NULL) {
            ERR("%s:get object item AssociatedEquips NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item AssociatedEquips NULL");
            return ret;
        }
        DB_Associated_Equip_T associated_equip_db;
        associated_equip_db.EquipType = get_json_int(associated_equip_json, "EquipType");
        associated_equip_db.EquipCode = get_json_string(associated_equip_json, "EquipCode");
        db_associated_equip_insert(associated_equip_db);
    }

    //数据版本
    string DataVersion = get_json_string(data_json, "DataVersion");
    DBG("receive DataVersion:%s", DataVersion.c_str());
    if (DataVersion.length() > 1) {
        ret = update_conf_version(DataVersion.c_str());
        if (ret != 0) {
            ERR("%s:update_conf_version() Err", __FUNCTION__);
        }
    } else {
        ERR("%s get PkaDataVersion err", __FUNCTION__);
        ret = -1;
    }

    if (ret < 0) {
        DBG("102 deal fail");
        ret = s102_config_download_send(client, comm_guid, 0, "fail");
    } else {
        DBG("102 deal success");
        ret = s102_config_download_send(client, comm_guid, 1, "ok");

        //    sleep(5);
        //    _exit(0);
        add_reboot_task();
    }

    return 0;
}

/*
 * 向eoc发送获取配置
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s104_get_config_send(tcp_client_t *client) {
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char *json_str = NULL;
    string send_str;
    string equip_num;
    ret = db_factory_get_uname(equip_num);
    if (ret != 0) {
        ERR("%s:get_factory_uname() Err", __FUNCTION__);
        return -1;
    }
//    equip_num = "aab65d77-8242-4f31-b088-46eb038663d9";

    //打包返回信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_104);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_104);
    cJSON_AddStringToObject(data, "MainboardGuid", equip_num.c_str());

    json_str = cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

static int r104_get_config_deal(tcp_client_t *client, cJSON *json) {
    int ret = 0;
    char guid[8] = {0};
    memset(guid, 0, 8);
    ret = r102_config_download_deal(client, (const char *) guid, json);

    return ret;
}

//取外网状态，total外网通信总次数，success通信成功次数
int get_net_status(int *total, int *success) {
    *total = 0;
    *success = 0;

    return 0;
}

/*
 * 外网状态上传
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s105_internet_status_send(tcp_client_t *client) {
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char *json_str = NULL;
    string send_str;

    //打包发送信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_105);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_105);
    int total_cnt = 0;
    int success_cnt = 0;
    ret = get_net_status(&total_cnt, &success_cnt);
    cJSON_AddNumberToObject(data, "Total", total_cnt);
    cJSON_AddNumberToObject(data, "Success", success_cnt);

    json_str = cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

static int r105_internet_status_deal(cJSON *json) {
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    int state = get_json_int(data_json, "State");

    if (state == 1) {
        DBG("外网状态上传成功");
    } else {
        ERR("外网状态上传失败, msg: %s", get_json_string(data_json, "Message").c_str());
    }

    return 0;
}

//多目下载、升级
int s106_download_send(tcp_client_t *client, const char *comm_guid,
                       int result, int stats, int progress, const char *msg) {
    int ret = -1;
    cJSON *root = NULL, *data = NULL;
    char *json_str = NULL;
    string send_str;

    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_106);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_106);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str = cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

/*
 * 处理多目软件下载文件指令，将会把文件下载到指定目录下
 * */
int r106_download_deal(tcp_client_t *client, const char *comm_guid, cJSON *json) {
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;

    cJSON *data = cJSON_GetObjectItem(json, "Data");

    EocDownloadsMsg rcv_download_msg;
    EocUpgradeDev rcv_dev_msg;
    rcv_download_msg.download_file_name = get_json_string(data, "Filename");
    rcv_download_msg.download_file_size = atoi(get_json_string(data, "FileSize").c_str());
    rcv_download_msg.download_url = get_json_string(data, "DownloadPath");
    rcv_download_msg.download_file_md5 = get_json_string(data, "FileMD5");
    rcv_dev_msg.dev_guid.clear();
    rcv_dev_msg.dev_ip.clear();
    rcv_dev_msg.dev_type = EOC_UPGRADE_PARKINGAREA;
    rcv_dev_msg.comm_guid = comm_guid;

    ret = s106_download_send(client, comm_guid,
                             1, 1, 0, "Receive controller download msg succeed");
    if (ret != 0)return ret;
    //判断文件是否存在，已存在直接返回下载成功
//    char update_file_name[256] = {0};
//    snprintf(update_file_name, 256, "%s_%s", rcv_download_msg.download_file_md5.c_str(),
//                rcv_download_msg.download_file_name.c_str());
    if (access(UPDATEFILE, R_OK | R_OK) == 0) {
        DBG("%s:%s 文件已存在", __FUNCTION__, UPDATEFILE);
        if (compute_file_md5(UPDATEFILE, rcv_download_msg.download_file_md5.c_str()) == 0) {
            DBG("下载文件MD5校验通过");
            return s106_download_send(client, comm_guid, 2, 1, 0, "下载完成 MD5 OK");
        }
    }
    //添加下载任务
    ret = add_eoc_download_event(rcv_download_msg, rcv_dev_msg);
    if (ret < 0) {
        DBG("添加下载任务失败");
        return s106_download_send(client, comm_guid, 2, 0, 0, "add download_event Err");
    } else if (1 == ret) {
        DBG("下载任务存在，返回错误");
        return s106_download_send(client, comm_guid, 2, 0, 0, "该下载正在进行");
    }

    return 0;
}

int s107_upgrade_send(tcp_client_t *client, const char *comm_guid,
                      int result, int stats, int progress, const char *msg) {
    int ret = -1;
    cJSON *root = NULL, *data = NULL;
    char *json_str = NULL;
    string send_str;

    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_107);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_107);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str = cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

static int extract_file(const char *file_path) {
    int ret = 0;
    char cmd[256] = {0};

    sprintf(cmd, "rm -rf %s", UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if (ret < 0) {
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "mkdir -p %s", UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if (ret != 0) {
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "tar -zxf %s -C %s", file_path, UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if (ret < 0) {
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    if (remove(file_path) != 0) {
        ERR("remove %s error", file_path);
        return -1;
    }

    DBG("%s:Extract file succeed", __FUNCTION__);
    return 0;
}

static int start_upgrade() {
    int ret = 0;
    DBG("%s:Enter.", __FUNCTION__);
    char cmd[256] = {0};

    sprintf(cmd, "chmod +x %s/%s", UPDATEUNZIPFILE, INSTALLSH);
    DBG("%s:cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if (ret < 0) {
        ERR("%s:system(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "sh %s/%s %s", UPDATEUNZIPFILE, INSTALLSH, HOME_PATH);
    DBG("%s:cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if (ret < 0) {
        ERR("%s:system(%s) Err", __FUNCTION__, cmd);
        return -1;
    }
    DBG("%s:succeed", __FUNCTION__);
    return 0;
}

//相机升级
int r107_upgrade_deal(tcp_client_t *client, const char *comm_guid, cJSON *json) {
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;
    EocUpgradeDev rcv_data;

    cJSON *data = cJSON_GetObjectItem(json, "Data");
    string file_md5 = get_json_string(data, "FileMD5");
    rcv_data.sw_version = get_json_string(data, "FileVersion");
    rcv_data.hw_version = get_json_string(data, "HardwareVersion");
    rcv_data.upgrade_mode = get_json_int(data, "UpgradeMode");
    string curr_sw_version = get_json_string(data, "CurrentSoftVersion");

    s107_upgrade_send(client, comm_guid, 1, 1, 0, "接收到升级命令");

    //1 判断文件是否存在
    if (access(UPDATEFILE, R_OK | R_OK) != 0) {
        ERR("%s:%s 文件不存在", __FUNCTION__, UPDATEFILE);
        return s107_upgrade_send(client, comm_guid, 2, 0, 0, "文件不存在");
    }
    //2 MD5校验
    if (compute_file_md5(UPDATEFILE, file_md5.c_str()) != 0) {
        ERR("%s:%s MD5校验失败", __FUNCTION__, UPDATEFILE);
        return s107_upgrade_send(client, comm_guid, 2, 0, 0, "MD5校验失败");
    }
    //3 版本校验... ...


    ret = s107_upgrade_send(client, comm_guid, 1, 1, 25, "校验完成开始升级");
    //4 执行升级文件
    if (extract_file(UPDATEFILE) != 0) {
        ERR("%s:%s 解压缩失败", __FUNCTION__, UPDATEFILE);
        return s107_upgrade_send(client, comm_guid, 2, 0, 0, "解压缩失败");
    }
    ret = s107_upgrade_send(client, comm_guid, 2, 1, 0, "升级完成");
    start_upgrade();
    return ret;
}

//向EOC返回下载结果
static int send_download_msg(tcp_client_t *client, EocDownloadsMsg *downloade_msg) {
    int ret = 0;

    for (size_t i = 0; i < downloade_msg->upgrade_dev.size(); i++) {
        if (downloade_msg->upgrade_dev[i].dev_type == EOC_UPGRADE_PARKINGAREA) {
            //软件升级
            if (downloade_msg->download_status == DOWNLOAD_FINISHED) {
                ret = s106_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 1, 0,
                                         "下载完成 MD5 OK");
            } else if (downloade_msg->download_status == DOWNLOAD_FILE_NOT_EXIST) {
                ret = s106_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0,
                                         "下载文件不存在");
            } else if (downloade_msg->download_status == DOWNLOAD_TIMEOUT) {
                ret = s106_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0, "下载超时");
            } else if (downloade_msg->download_status == DOWNLOAD_SPACE_NOT_ENOUGH) {
                ret = s106_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0,
                                         "本地剩余空间不足");
            } else if (downloade_msg->download_status == DOWNLOAD_MD5_FAILD) {
                ret = s106_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0,
                                         "MD5校验失败");
            }
        }

        if (ret != 0)
            break;
    }

    return ret;
}

/*
 * 主控机状态上传
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s103_mainBoard_status_send(tcp_client_t *client) {
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *_MainBoardState = NULL;
    char *json_str = NULL;
    string send_str;

    //打包发送信息
    root = cJSON_CreateObject();
    data = cJSON_CreateObject();
    _MainBoardState = cJSON_CreateObject();
    if (root == NULL || data == NULL) {
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if (root)cJSON_Delete(root);
        if (data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_103);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_103);

    //填充data

    MainBoardState mainBoardState;
    GetMainBoardState(mainBoardState);

    cJSON_AddStringToObject(_MainBoardState, "MainboardGuid", mainBoardState.MainboardGuid.c_str());
    cJSON_AddNumberToObject(_MainBoardState, "State", mainBoardState.State);
    cJSON_AddNumberToObject(_MainBoardState, "CpuState", mainBoardState.CpuState);
    cJSON_AddNumberToObject(_MainBoardState, "CpuUtilizationRatio", mainBoardState.CpuUtilizationRatio);
    cJSON_AddNumberToObject(_MainBoardState, "CpuTemperature", mainBoardState.CpuTemperature);
    cJSON_AddNumberToObject(_MainBoardState, "MemorySize", mainBoardState.MemorySize);
    cJSON_AddNumberToObject(_MainBoardState, "ResidualMemorySize", mainBoardState.ResidualMemorySize);
    cJSON_AddStringToObject(_MainBoardState, "ModelVersion", mainBoardState.ModelVersion.c_str());
    cJSON_AddStringToObject(_MainBoardState, "MainboardType", mainBoardState.MainboardType.c_str());

    cJSON *array_tf = cJSON_CreateArray();
    for (int i = 0; i < mainBoardState.TFCardStates.size(); i++) {
        cJSON *array_tf_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(array_tf_item, "State", mainBoardState.TFCardStates[i].State);
        cJSON_AddNumberToObject(array_tf_item, "Size", mainBoardState.TFCardStates[i].Size);
        cJSON_AddNumberToObject(array_tf_item, "ResidualSize", mainBoardState.TFCardStates[i].ResidualSize);
        cJSON_AddItemToArray(array_tf, array_tf_item);
    }
    cJSON_AddItemToObject(_MainBoardState, "TFCardStates", array_tf);

    cJSON *array_emmc = cJSON_CreateArray();
    for (int i = 0; i < mainBoardState.EmmcStates.size(); i++) {
        cJSON *array_emmc_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(array_emmc_item, "State", mainBoardState.EmmcStates[i].State);
        cJSON_AddNumberToObject(array_emmc_item, "Size", mainBoardState.EmmcStates[i].Size);
        cJSON_AddNumberToObject(array_emmc_item, "ResidualSize", mainBoardState.EmmcStates[i].ResidualSize);
        cJSON_AddItemToArray(array_emmc, array_emmc_item);
    }
    cJSON_AddItemToObject(_MainBoardState, "EmmcStates", array_emmc);

    cJSON *array_externalHardDisk = cJSON_CreateArray();
    for (int i = 0; i < mainBoardState.ExternalHardDisk.size(); i++) {
        cJSON *array_xternalHardDisk_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(array_xternalHardDisk_item, "State", mainBoardState.ExternalHardDisk[i].State);
        cJSON_AddNumberToObject(array_xternalHardDisk_item, "Size", mainBoardState.ExternalHardDisk[i].Size);
        cJSON_AddNumberToObject(array_xternalHardDisk_item, "ResidualSize",
                                mainBoardState.ExternalHardDisk[i].ResidualSize);
        cJSON_AddItemToArray(array_externalHardDisk, array_xternalHardDisk_item);
    }
    cJSON_AddItemToObject(_MainBoardState, "ExternalHardDisk", array_externalHardDisk);
    cJSON_AddItemToObject(data, "MainBoardState", _MainBoardState);
    json_str = cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    printf("s103 send %s\n", send_str.c_str());
    ret = tcp_client_write(client, (char *) send_str.c_str(), send_str.length());
    if (ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    } else {
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}


//eoc通信数据初始化eoc_comm_data
int comm_data_init(void) {
    eoc_comm_data.is_running = true;
    eoc_comm_data.downloads_msg.clear();
    eoc_comm_data.upgrade_msg.clear();
    eoc_comm_data.task.clear();
    eoc_comm_data.eoc_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
    eoc_rec_data.clear();
    return 0;
}

/*
 * TCP建立连接后调用函数
 *
 * 参数：
 * client：连接的socket
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
BLUE_TCP_STATE connected_callback(tcp_client_t *client) {
    /*
     * 1、初始化EOC状态
     * */
    is_login = false;
    heart_flag = 0;
    eoc_rec_data.clear();
    comm_data_init();

    int ret = 0;
    ret = s101_login_send(client);
    if (ret < 0) {
        ERR("s101_login_send err, return %d", ret);
        return (BLUE_TCP_STATE) 1;
    } else {
        DBG("s101_login_send ok");
    }

    db_version_get(db_conf_version);
#if 0
    /*
     * 2、清空请求列表
     * */
    clear_request();

    /*
     * 3、发送登录请求
     * */
    string u;
    if(get_uname(u) == 0){
        if(login(u) == 0){
            add_request("", "login");
            return (BLUE_TCP_STATE)0;
        }
    }
#endif
    return (BLUE_TCP_STATE) 0;
}

/*
 * TCP有数据进来回调函数
 *
 * 参数：
 * client：连接的socket
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
BLUE_TCP_STATE datacoming_callback(tcp_client_t *client, char *data, size_t data_size) {
    if (strlen(data) <= 0) {
        ERR("receive data null");
        return (BLUE_TCP_STATE) 0;
    }
    if (data[strlen(data) - 1] != '*') {
        eoc_rec_data += data;
        DBG("接收不全%s", data);
        return (BLUE_TCP_STATE) 0;
    }
    eoc_rec_data += data;

    int ret = 0;
    vector<string> sv;
    //处理粘包
    ret = splitstr2vector(eoc_rec_data.c_str(), "*", sv);
    if (ret == 0) {
//      DBG("buf=%s", buf);
        for (string &real_json_str: sv) {
            cJSON *json;
            char *rec_out;
            char code[10], guid[37];
            char *tmpbuf;

            int buf_len = (int) strlen(real_json_str.c_str());
            int rec_out_len = 0;

//          DBG("real_json_str=%s", real_json_str.c_str());
            //数据解析
            json = cJSON_Parse(real_json_str.c_str());//字符串解析成json结构体
            if (json == NULL) {
                ERR("Error before: [%s]", cJSON_GetErrorPtr());
                ERR("不是json结构体:%s", real_json_str.c_str());
                continue;
            }

            rec_out = cJSON_PrintUnformatted(json);//json结构体转换成字符串
            rec_out_len = (int) strlen(rec_out);
            //    DBG("buf strlen %d, rec_out strlen %d", buf_len, rec_out_len);

            sprintf(code, "%s", get_json_string(json, "Code").c_str());//读取指令码

            if (strcmp(code, EOC_COMM_RECEIVE_100) == 0) {
                ret = r100_heart_deal(json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_101) == 0) {
                ret = r101_login_deal(json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_102) == 0) {
                ret = r102_config_download_deal(client, get_json_string(json, "Guid").c_str(), json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_104) == 0) {
                ret = r104_get_config_deal(client, json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_105) == 0) {
                ret = r105_internet_status_deal(json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_106) == 0) {
                ret = r106_download_deal(client, get_json_string(json, "Guid").c_str(), json);
            } else if (strcmp(code, EOC_COMM_RECEIVE_107) == 0) {
                ret = r107_upgrade_deal(client, get_json_string(json, "Guid").c_str(), json);
            } else if (strcmp(code, "MCCR103") == 0) {
                //打印下内容
                DBG("%s:103:%s", __FUNCTION__, get_json_string(json, "Data").c_str());
            } else {
                WARNING("%s:UNKNOWN _code[%s]", __FUNCTION__, code);
            }
            //最后退出
            cJSON_Delete(json);
            free(rec_out);
        }
    } else {
        ERR("%s splitstr2vector() Err", __FUNCTION__);
        ret = 0;
    }
    eoc_rec_data.clear();

#if 0
    /*
     * 1、解析命令
     * */
    string cmd;

    /*
     * 2、处理命令
     * */
    if(cmd == "login"){
        del_request("", "login");
    }

    /*
     * 3、发送命令返回
     * */
#endif
    if (ret < 0) {
        return (BLUE_TCP_STATE) 1;
    } else {
        return (BLUE_TCP_STATE) 0;
    }
}

/*
 * 周期回调函数
 *
 * 参数：
 * fd：连接的socket fd
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
BLUE_TCP_STATE interval_callback(tcp_client_t *client) {
    int ret = 0;
    static time_t last_login_t = time(NULL);
    static time_t last_send_heart_t = time(NULL);
    static time_t send_net_state_time = time(NULL) - 250;   //外网状态
    static time_t get_config_time = 0;
    static time_t send_mainBoard_state_time = time(NULL) - 250;   //主控机状态

    time_t now = time(NULL);

    if (is_login == false && now - last_login_t > 10) {
        last_login_t = now;
        ret = s101_login_send(client);
        if (ret < 0) {
            ERR("s101_login_send err, return %d", ret);
            return (BLUE_TCP_STATE) 1;
        } else {
            DBG("s101_login_send ok");
        }
        last_send_heart_t = now - 20;
    }

    if (is_login == true) {
        if ((now - get_config_time > 180) && (db_conf_version.length() == 0)) {
            get_config_time = now;

            DBG("s104_get_config_send");
            ret = s104_get_config_send(client);
            if (ret < 0) {
                ERR("s104_get_config_send err, return %d", ret);
                return (BLUE_TCP_STATE) 1;
            } else {
                DBG("s104_get_config_send ok");
            }
        }
        //心跳
        if (now - last_send_heart_t > 30) {
            last_send_heart_t = now;
            /*
             * 发送心跳状态
             * */
            ret = s100_heart_send(client);
            if (ret < 0) {
                ERR("s100_heart_send err, return %d", ret);
                return (BLUE_TCP_STATE) 1;
            } else {
                DBG("s100_heart_send ok");
            }
            heart_flag++;
            if (heart_flag > 10) {
                ERR("心跳连续10次未回复");
                return (BLUE_TCP_STATE) 1;
            }
        }
        //外网状态上传
        if (now - send_net_state_time > 300) {
            send_net_state_time = now;
            ret = s105_internet_status_send(client);
            if (ret < 0) {
                ERR("internet_status_send err, return %d", ret);
                return (BLUE_TCP_STATE) 1;
            } else {
                DBG("internet_status_send ok");
            }
        }
        //主控机状态上传
        if (now - send_mainBoard_state_time > 60) {
            send_mainBoard_state_time = now;
            ret = s103_mainBoard_status_send(client);
            if (ret < 0) {
                ERR("s103_mainBoard_status_send err, return %d", ret);
                return (BLUE_TCP_STATE) 1;
            } else {
                DBG("s103_mainBoard_status_send ok");
            }
        }


        //升级、下载
        //处理下载消息
        ret = 0;
        for (auto it = eoc_comm_data.downloads_msg.begin(); it != eoc_comm_data.downloads_msg.end();) {
            if (it->download_status != DOWNLOAD_IDLE) {
                ret = send_download_msg(client, &(*it));
                it = eoc_comm_data.downloads_msg.erase(it); //删除元素，返回值指向已删除元素的下一个位置
                if (ret != 0) {
                    ERR("send_download_msg() err, ret=%d", ret);
                    break;
                }
            } else {
                ++it; //指向下一个位置    
            }
        }

        //任务处理
        if (eoc_comm_data.task.size()) {
            EOCCLOUD_TASK task = eoc_comm_data.task[0];
            eoc_comm_data.task.erase(eoc_comm_data.task.begin());
            if (task == SYS_REBOOT) {
                if ((eoc_comm_data.task.size() > 0) || (eoc_comm_data.downloads_msg.size() > 0)) {
                    DBG("有其他任务正在进行，延后重启");
                    //重新添加重启任务
                    add_reboot_task();
                } else {
                    DBG("running taskI SYS_REBOOT");
                    sleep(5);
                    _exit(0);
                }
            } else {
                DBG("taskI = %d unsupport", (int) task);
            }
        }

    }
#if 0
    static time_t last_login_t = 0;
    static time_t last_send_gps_t = 0;
    static time_t last_send_5g_t = 0;
    static time_t last_send_heart_t = 0;

    if(get_unhandle_request() > 5){
        return (BLUE_TCP_STATE)1;
    }

    time_t now = time(NULL);

    if(is_login == false && now - last_login_t > 5){
        last_login_t = now;
        string u;
        get_uname(u);
        login(u);
        add_request("", "login");
    }

    if(now - last_send_gps_t > 60){
        last_send_gps_t = now;
        /*
         * 发送GPS状态
         * */
        add_request("", "GPS");
    }

    if(now - last_send_heart_t > 60){
        last_send_heart_t = now;
        /*
         * 发送心跳状态
         * */
        add_request("", "heart");
    }
#endif
    return (BLUE_TCP_STATE) 0;
}

/*
 * TCP断开连接后回调函数
 *
 * 参数：无
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
BLUE_TCP_STATE disconnected_callback() {
    DBG("disconnected_callback");
    return (BLUE_TCP_STATE) 0;
}


int eoc_communication_start(const char *server_path, int server_port) {
    DBG("EOC communication Running");
    tcp_client_t *client = tcp_client_create("yylsj-eoc communication", server_path, server_port, connected_callback,
                                             datacoming_callback, disconnected_callback, interval_callback, 1);

    return 0;
}

std::string getValueBySystemCommand(std::string strCommand){
    //printf("-----%s------\n",strCommand.c_str());
    std::string strData = "";
    FILE * fp = NULL;
    char buffer[128];
    fp=popen(strCommand.c_str(),"r");
    if (fp) {
        while(fgets(buffer,sizeof(buffer),fp)){
            strData.append(buffer);
        }
        pclose(fp);
    }
    //printf("-----%s------ end\n",strCommand.c_str());
    return strData;
}


int runSystemCommand(std::string strCommand){
//    printf("-----%s------\n",strCommand.c_str());
    return system(strCommand.c_str());
}

string trim(string str)
{
    if (str.empty())
    {
        return str;
    }
    str.erase(0,str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

std::vector<std::string> split(std::string str, std::string pattern)
{
    string::size_type pos;
    vector<string> result;
    str += pattern;
    int size = str.size();

    for (int i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result;
}


static double CpuUtilizationRatio() {

    std::string strRes = getValueBySystemCommand("top -b -n 1 |grep Cpu | cut -d \",\" -f 1 | cut -d \":\" -f 2");

    strRes = trim(strRes);
    printf("--%s--  strRes.size : %lu \n", strRes.c_str(),strRes.length());
    std::vector<std::string> vecT = split(strRes," ");
    if (vecT.size() == 2){
        printf("%s\n", vecT.at(0).c_str());
        return atof(vecT.at(0).c_str());
    }
    return 0;
}


static double CpuTemperature() {
    FILE *fp = NULL;
    int temp = 0;
    fp = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
    if (fp == nullptr) {
        std::cerr << "CpuTemperature fail" << std::endl;
        return 0;
    }

    fscanf(fp, "%d", &temp);
    fclose(fp);
    return (double) temp / 1000;
}

static double MemorySize() {
    int mem_free = -1;//空闲的内存，=总内存-使用了的内存
    int mem_total = -1; //当前系统可用总内存
    int mem_buffers = -1;//缓存区的内存大小
    int mem_cached = -1;//缓存区的内存大小
    char name[20];

    FILE *fp;
    char buf1[128], buf2[128], buf3[128], buf4[128], buf5[128];
    int buff_len = 128;
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        std::cerr << "GetSysMemInfo() error! file not exist" << std::endl;
        return -1;
    }
    if (NULL == fgets(buf1, buff_len, fp) ||
        NULL == fgets(buf2, buff_len, fp) ||
        NULL == fgets(buf3, buff_len, fp) ||
        NULL == fgets(buf4, buff_len, fp) ||
        NULL == fgets(buf5, buff_len, fp)) {
        std::cerr << "GetSysMemInfo() error! fail to read!" << std::endl;
        fclose(fp);
        return -1;
    }
    fclose(fp);
    sscanf(buf1, "%s%d", name, &mem_total);
    sscanf(buf2, "%s%d", name, &mem_free);
    sscanf(buf4, "%s%d", name, &mem_buffers);
    sscanf(buf5, "%s%d", name, &mem_cached);
    return (double) mem_total / 1024.0;
}

static double ResidualMemorySize() {
    int mem_free = -1;//空闲的内存，=总内存-使用了的内存
    int mem_total = -1; //当前系统可用总内存
    int mem_buffers = -1;//缓存区的内存大小
    int mem_cached = -1;//缓存区的内存大小
    char name[20];

    FILE *fp;
    char buf1[128], buf2[128], buf3[128], buf4[128], buf5[128];
    int buff_len = 128;
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        std::cerr << "GetSysMemInfo() error! file not exist" << std::endl;
        return -1;
    }
    if (NULL == fgets(buf1, buff_len, fp) ||
        NULL == fgets(buf2, buff_len, fp) ||
        NULL == fgets(buf3, buff_len, fp) ||
        NULL == fgets(buf4, buff_len, fp) ||
        NULL == fgets(buf5, buff_len, fp)) {
        std::cerr << "GetSysMemInfo() error! fail to read!" << std::endl;
        fclose(fp);
        return -1;
    }
    fclose(fp);
    sscanf(buf1, "%s%d", name, &mem_total);
    sscanf(buf2, "%s%d", name, &mem_free);
    sscanf(buf4, "%s%d", name, &mem_buffers);
    sscanf(buf5, "%s%d", name, &mem_cached);
    return (double) mem_free / 1024.0;
}

#include <sys/statfs.h>

static double TF_Size() {
    string devStr = "~/mnt_tf";
    struct statfs diskInfo;
    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) totalsize / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

static double TF_ResidualSize() {
    string devStr = "~/mnt_tf";
    struct statfs diskInfo;

    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) freeDisk / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

static double EMMC_Size() {
    string devStr = "/dev/mmcblk0p1";
    struct statfs diskInfo;

    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) totalsize / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

static double EMMC_ResidualSize() {
    string devStr = "/dev/mmcblk0p1";
    struct statfs diskInfo;

    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) freeDisk / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

static double ExternalHardDisk_Size() {
    string devStr = "/dev/sda1";
    struct statfs diskInfo;

    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) totalsize / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

static double ExternalHardDisk_ResidualSize() {
    string devStr = "/dev/sda1";
    struct statfs diskInfo;

    // 设备挂载的节点
    if (statfs(devStr.c_str(), &diskInfo) == 0) {
        uint64_t blocksize = diskInfo.f_bsize;                   // 每一个block里包含的字节数
        uint64_t totalsize = blocksize * diskInfo.f_blocks;      // 总的字节数，f_blocks为block的数目
        uint64_t freeDisk = diskInfo.f_bfree * blocksize;       // 剩余空间的大小
        uint64_t availableDisk = diskInfo.f_bavail * blocksize; // 可用空间大小
        return (double) freeDisk / (1024.0 * 1024.0);
    } else {
        return 0;
    }
}

int GetMainBoardState(MainBoardState &mainBoardState) {
    string equip_num;
    db_factory_get_uname(equip_num);
    mainBoardState.MainboardGuid = equip_num;

    mainBoardState.State = 1;

    mainBoardState.CpuState = 1;

    //cpu利用率
    mainBoardState.CpuUtilizationRatio = CpuUtilizationRatio();

    //cpu温度
    mainBoardState.CpuTemperature = CpuTemperature();

    //内存大小
    mainBoardState.MemorySize = MemorySize();

    //剩余内存大小
    mainBoardState.ResidualMemorySize = ResidualMemorySize();

    //模型版本
    mainBoardState.ModelVersion = "NX";

    //主控版类型
    mainBoardState.MainboardType = "NX";

    //TF卡状态
    {
        PartsState state;
        state.State = 1;
        state.Size = TF_Size();
        state.ResidualSize = TF_ResidualSize();
        mainBoardState.TFCardStates.push_back(state);
    }

    //emmc状态
    {
        PartsState state;
        state.State = 1;
        state.Size = EMMC_Size();
        state.ResidualSize = EMMC_ResidualSize();
        mainBoardState.EmmcStates.push_back(state);
    }

    //外接硬盘状态
    {
        PartsState state;
        state.State = 1;
        state.Size = ExternalHardDisk_Size();
        state.ResidualSize = ExternalHardDisk_ResidualSize();
        mainBoardState.ExternalHardDisk.push_back(state);
    }

    return 0;
}

