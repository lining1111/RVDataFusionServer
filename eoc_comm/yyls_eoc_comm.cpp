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
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/highgui/highgui.hpp>
#include <signal.h>

#include "cJSON.h"
#include "yyls_eoc_comm.hpp"
#include "sqlite_eoc_db.h"
#include "configure_eoc_init.h"
#include "base64.h"
#include "dns_server.h"
#include "cck1_eoc_api.hpp"
#include "md5.h"
#include "MySystemCmd.hpp"
//#include "camera/xm_camera_business.hpp"
//#include "camera/xm_camera.hpp"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "logger.h"

//using namespace cv;
using namespace std;


extern EOC_Radar_Info g_eoc_radar_info;
extern EOC_Base_Set g_eoc_base_set;    //基础配置


//#define LICENSEFILE "ls.lic"
#define UPDATEFILE  "ls_update.tgz"
#define UPDATEUNZIPFILE HOME_PATH "/ls_update_tmp"  // 升级包解压路径
#define INSTALLSH "update.sh"  // 升级脚本
//#define ZIPPASSWORD "!SaFe5￥Emb1*@Cv4"

static EOCCloudData eoc_comm_data;

class LockGuard
{
public:
	LockGuard(pthread_mutex_t *lock):
		lock_(lock)
	{
		pthread_mutex_lock(lock_);
	}
	virtual ~LockGuard()
	{
		pthread_mutex_unlock(lock_);
	}
private:
 	pthread_mutex_t *lock_;	
};


static vector<UP_EOC_CameraState> up_camera_state;  //上传eoc相机状态
pthread_mutex_t camera_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static UP_EOC_RadarState up_radar_state;  //上传eoc雷达状态
pthread_mutex_t radar_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static vector<Real_Time_Camera_Info> real_time_upload_cameras; //实时上传图片相机
pthread_mutex_t real_time_camera_mutex = PTHREAD_MUTEX_INITIALIZER;
int clear_upload_camera()
{
    LockGuard lock_gurad(&real_time_camera_mutex);
    real_time_upload_cameras.clear();
    return 0;
}
//实时上传图片相机
//返回值：0添加完成，2已存在
int add_upload_pic_camera(Real_Time_Camera_Info &data)
{
    LockGuard lock_gurad(&real_time_camera_mutex);
    unsigned int i = 0;
    for(i=0; i<real_time_upload_cameras.size(); i++)
    {
        if(data.CameraIp == real_time_upload_cameras[i].CameraIp)
        {
            DBG("%s相机已在上传", data.CameraIp.c_str());
            return 2;
        }
    }

    if(i == real_time_upload_cameras.size())
    {
        DBG("添加实时图上传相机%s", data.CameraIp.c_str());
        real_time_upload_cameras.push_back(data);
    }
    return 0;
}
//更新上传实时图相机上传状态
int update_upload_camera_state(const char* camera_ip, int state)
{
    LockGuard lock_gurad(&real_time_camera_mutex);
    for(unsigned int i=0; i<real_time_upload_cameras.size(); i++)
    {
        if(real_time_upload_cameras[i].CameraIp == camera_ip)
        {
            real_time_upload_cameras[i].Enable = state;
            break;
        }
    }
    return 0;
}
int get_upload_pic_camera(vector <Real_Time_Camera_Info> &data)
{
    LockGuard lock_gurad(&real_time_camera_mutex);
    data.clear();
    data = real_time_upload_cameras;
    return 0;
}
//返回相机是否继续上传实时图，1继续上传，0停止上传
int get_upload_camera_state(const char* camera_ip)
{
    int ret = 0;
    LockGuard lock_gurad(&real_time_camera_mutex);
    unsigned int i = 0;
    for(auto it = real_time_upload_cameras.begin(); it != real_time_upload_cameras.end();)
    {
        if(it->CameraIp == camera_ip)
        {
            if(it->Enable == 0)
            {
                //停止上传
                ret = 0;
                real_time_upload_cameras.erase(it);
            }
            else
            {
                ret = 1;
            }
            break;
        }
        ++it; //指向下一个位置
    }

    return ret;
}

//相机状态
//int update_camera_state(UP_EOC_CameraState data)
//{
//    LockGuard lock_gurad(&camera_state_mutex);
//    unsigned int i = 0;
//    for(i=0; i<up_camera_state.size(); i++)
//    {
//        if(data.Guid == up_camera_state[i].Guid)
//        {
//            up_camera_state[i].State = data.State;
//            up_camera_state[i].Model = data.Model;
//            up_camera_state[i].SoftVersion = data.SoftVersion;
//            up_camera_state[i].GetPicTotalCount = data.GetPicTotalCount;
//            up_camera_state[i].GetPicSuccessCount = data.GetPicSuccessCount;
//            break;
//        }
//    }
//
//    if(i == up_camera_state.size())
//    {
//        if(data.DataVersion == "")
//        {
//            char version[64] = {0};
//            if(1 == db_camera_detail_get_version((char*)data.Guid.c_str(), version))
//            {
//                data.DataVersion = version;
//            }
//        }
//        up_camera_state.push_back(data);
//        //添加可升级雄迈相机
//        if('X'== data.Model.c_str()[0])
//        {
//            std::string camera_ip = get_camera_ip(data.Guid.c_str());
//            xm_camera_add(camera_ip.c_str(), 34567, "admin", "aipark");
//        }
//    }
//
//    return 0;
//}
int get_up_camera_state(vector<UP_EOC_CameraState> &data)
{
    LockGuard lock_gurad(&camera_state_mutex);
    data = up_camera_state;
    
    return data.size();
}
int clear_up_camera_state()
{
    LockGuard lock_gurad(&camera_state_mutex);
    up_camera_state.clear();
    return 0;
}
int update_camera_data_version(const char *camera_guid, const char *data_ver)
{
    LockGuard lock_gurad(&camera_state_mutex);
    unsigned int i = 0;
    for(i=0; i<up_camera_state.size(); i++)
    {
        if(up_camera_state[i].Guid == camera_guid)
        {
            up_camera_state[i].DataVersion = data_ver;
            break;
        }
    }

    if(i == up_camera_state.size())
    {
        UP_EOC_CameraState data;
        data.Guid = camera_guid;
        data.State = 0;
        data.Model = "";
        data.SoftVersion = "";
        data.DataVersion = data_ver;
        up_camera_state.push_back(data);
    }

    return 0;
}
//雷达状态
/***更新雷达状态
** radar_guid:雷达 Guid
** state:雷达状态：0=正常，1异常
** soft_version:固件版本
**/
int update_radar_state(const char* radar_guid, int state, const char* soft_version)
{
    LockGuard lock_gurad(&radar_state_mutex);
    up_radar_state.Guid = radar_guid;
    up_radar_state.State = state;
    up_radar_state.SoftVersion = soft_version;

    return 0;
}
int get_up_radar_state(UP_EOC_RadarState &data)
{
    LockGuard lock_gurad(&radar_state_mutex);
    data.Guid = up_radar_state.Guid;
    data.State = up_radar_state.State;
    data.SoftVersion = up_radar_state.SoftVersion;
    
    return 0;
}
int clear_up_radar_state()
{
    LockGuard lock_gurad(&radar_state_mutex);
    up_radar_state.Guid = g_eoc_radar_info.Guid;
    up_radar_state.State = 0;
    up_radar_state.SoftVersion = "";
    return 0;
}



/*取板卡本地ip地址和n2n地址 */
int getipaddr (char *ethip, char *n2nip)
{
    int ret = 0;
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * pifAddrStruct = NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    if (ifAddrStruct != NULL )
    {
        pifAddrStruct = ifAddrStruct;
    }

    while (ifAddrStruct!=NULL)
    {
        if(ifAddrStruct->ifa_addr == NULL)
        {
            ifAddrStruct=ifAddrStruct->ifa_next;
            DBG("addr NULL!");
            continue;
        }
        
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)
        { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            if(strcmp(ifAddrStruct->ifa_name, "eth0") == 0)
            {
                DBG("eth0 IP Address %s", addressBuffer);
                sprintf(ethip, "%s", addressBuffer);
            }
            else if(strcmp(ifAddrStruct->ifa_name, "n2n0") == 0)
            {
                DBG("n2n0 IP Address %s", addressBuffer);
                sprintf(n2nip, "%s", addressBuffer);
            } 
        } 
        
        ifAddrStruct=ifAddrStruct->ifa_next;
    }

    if (pifAddrStruct != NULL)
    {
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
int splitstr2vector(const char* str, const char* delimiters, std::vector<std::string> &sv)
{
	sv.clear();

	char *ptr = NULL;
    char *p = NULL;
    char *str_tmp = (char*)malloc(strlen(str) + 1);
    if(str_tmp == NULL){
        ERR("%s:malloc() Error\n", __FUNCTION__);
        return -1;
    }

    sprintf(str_tmp, "%s", str);
    ptr = strtok_r(str_tmp, delimiters, &p);
    while(ptr != NULL){
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
static string random_uuid()
{
    char buf[37] = {0};
    struct timeval tmp;
    const char *c = "89ab";
    char *p = buf;
    unsigned int n,b;
    gettimeofday(&tmp, NULL);
    srand(tmp.tv_usec);

    for(n = 0; n < 16; ++n){
        b = rand()%65536;
        switch(n){
            case 6:
                sprintf(p, "4%x", b%15);
                break;
            case 8:
                sprintf(p, "%c%x", c[rand()%strlen(c)], b%15);
                break;
            default:
                sprintf(p, "%02x", b);
                break;
        }
        p += 2;
        switch(n){
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
 * 函数功能：获取本机MAC地址
 * 参数：
 * mac_buff：获取的MAC地址存放地址（50:7B:9D:E7:5E:8A = 18），长度20字节
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int get_mac(char * mac_buff)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[2048];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1) {
        ERR("socket error");
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        ERR("ioctl error");
        close(sock);
        return -1;
    }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    char szMac[64];
    int count = 0;
    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    count++;
                    unsigned char * ptr ;
                    ptr = (unsigned char  *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                    snprintf(szMac,64,"%02x:%02x:%02x:%02x:%02x:%02x",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
                    DBG("%d,Interface name:%s,Mac address:%s", count, ifr.ifr_name, szMac);
                    sprintf(mac_buff, "%s", szMac);
                    break;
                }
            }
        }else{
            ERR("get mac info error");
            close(sock);
            return -1;
        }
    }

    close(sock);
    return 0;
}

/*
 * 函数功能：http中post使用的CURLOPT_WRITEFUNCTION
 * */
static size_t http_write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    vector<char> *data_back = (vector<char> *)userp;
    for(unsigned int i=0;i<nmemb;i++){
        data_back->push_back(((char*)buffer)[i]);
    }
    return nmemb;
}
/*
 * 函数功能：http使用post方式上传数据
 *
 * 参数：
 * url:请求唯一标识符
 * post_filelds:需要上传的数据
 * connect_timeout_ms:连接超时时间(如果传入-1则不设置，使用默认超时时间)
 * timeout_ms:接受数据超时时间(如果传入-1则不设置，使用默认超时时间)
 * data_back:返回的数据(后面会多一个0x00)
 *
 * 返回值：
 * 0：上传成功
 * -1：上传失败
 * */
static int http_post_filelds(const char *url, const char *post_filelds, 
                           int connect_timeout_ms, int timeout_ms, 
                           vector<char> &data_back,std::string host,std::string port)
{
    int ret = 0;
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl){
        struct curl_slist *headers = NULL;
        //增加HTTP header
        headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
        host = "Host:" + host;
        port = "Port:" + port;
        headers = curl_slist_append(headers, "Expect:");
        //headerlist = curl_slist_append(headerlist, "Accept: application/json");
        headers = curl_slist_append(headers, host.c_str());
        headers = curl_slist_append(headers, port.c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);                               //查找次数，防止查找太深
        if(connect_timeout_ms > 0){
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);  //连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
        }
        if(timeout_ms > 0){
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);                 //接收数据时超时设置，如果10秒内数据未接收完，直接退出
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_filelds);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&data_back);
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK){
            ERR("http_post_filelds() curl_easy_perform() failed: %s",curl_easy_strerror(res));
            ret = -1;
        }else{
            //请求成功
            data_back.push_back(0);
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return ret;
}
/*
 * 函数功能：获取当前时间
 * */
static string get_current_time()
{
    time_t now;
    struct tm timenow;
    char p[50] = {0};

    time(&now);
    localtime_r(&now, &timenow);
    sprintf(p, "%d%02d%02d%02d%02d%02d",
            1900+timenow.tm_year,
            1+timenow.tm_mon,
            timenow.tm_mday,
            timenow.tm_hour,
            timenow.tm_min,
            timenow.tm_sec);

    return string(p);
}
/*
 * 函数功能：把图片进行base64编码并返回其string
 * 参数：
 * name：图片path
 * 返回值：
 * 图片的base64编码
 * */
static string image2base64(const char *pic_path)
{
    string ret;

    ifstream in_file;
    int length;
    in_file.open(pic_path);                     // open input file
    if(in_file.is_open()){
        in_file.seekg(0, std::ios::end);    // go to the end
        length = in_file.tellg();           // report location (this is the length)
        in_file.seekg(0, std::ios::beg);    // go back to the beginning
        char *buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
        in_file.read(buffer, length);       // read the whole file into the buffer
        in_file.close();                    // close file handle
        //base64
        char *base64 = new char[length*2];  // allocate memory for a buffer of appropriate dimension

        base64_encode((const unsigned char*)buffer, length, (unsigned char*)base64);

        ret = string(base64);

        delete buffer;
        delete base64;
    }else{
        DBG("image:%s.size()=%d", pic_path, 0);
    }

    return ret;
}
/*
 * 函数功能：上传图片到图片服务器
 *
 * 参数：
 * url：图片服务器请求URL
 * pic_path：图片路径
 * pic_url：上传成功后图片服务器返回的图片在服务器上的地址
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int send_picture_to_fileserver(const char *pic_path, string &pic_url)
{
    char url[128] = {0};
    int ret = 0;
    string t = get_current_time();
    string img_base64 = image2base64(pic_path);
    DBG("img_base64.size=%d", (int)img_base64.size());
    string json_data;
    cJSON *root = NULL;
    vector<char> data_back;

    std::string ipurl;
    std::string host,port;
    char server_file_path[128] = "ehctest.eoc.aipark.com";    //g_eoc_base_set.FilesServicePath//ceshi
    int server_file_port = 7100;                        //g_eoc_base_set.FilesServicePort//ceshi
    if((strlen(g_eoc_base_set.FilesServicePath)>0) && g_eoc_base_set.FilesServicePort>0)
    {
        snprintf(server_file_path, 128, "%s", g_eoc_base_set.FilesServicePath);
        server_file_port = g_eoc_base_set.FilesServicePort;
    }
    search_DNS_ipurl(server_file_path,ipurl,0,host,port);

    sprintf(url, "http://%s:%d/api/PicsUpload", (char *)ipurl.c_str(), server_file_port);
    root = cJSON_CreateObject();
    if(root){
        cJSON_AddStringToObject(root, "Guid", random_uuid().data());
        cJSON_AddStringToObject(root, "Pic_Server", "");
        cJSON_AddStringToObject(root, "Pic_Camera", "");
        cJSON_AddStringToObject(root, "Pic_ShutDate", t.data());
        cJSON_AddStringToObject(root, "Pic_CreateDate", t.data());
        cJSON_AddStringToObject(root, "Pic_Name", "1646123570.jpg");
        cJSON_AddStringToObject(root, "Pic_MD5", "");
        cJSON_AddStringToObject(root, "Pic_Content", img_base64.data());
        cJSON_AddNumberToObject(root, "Pic_Size", (int)img_base64.size());
        cJSON_AddStringToObject(root, "Pic_Extension", ".jpg");
        cJSON_AddStringToObject(root, "Pic_Explain", "01");

        char *s = cJSON_PrintUnformatted(root);
        if(s){
            json_data = string(s);
            free(s);
        }else{
            ERR("cJSON_PrintUnformatted failed.");
            ret = -1;
        }
        cJSON_Delete(root);
    }else{
        ERR("get cJSON *root failed.");
        ret = -1;
    }

    if(ret == 0){
        ret = http_post_filelds(url, json_data.data(), 30*1000, 30*1000, data_back,
                                         host,port);
        if(ret == 0){
            ret = -1;
            cJSON *json;
            json = cJSON_Parse(data_back.data());
            if(json){
                cJSON *json_ResultCode;
                json_ResultCode = cJSON_GetObjectItem(json, "ResultCode");
                if(json_ResultCode && json_ResultCode->type == cJSON_String){
                    if(strcmp(json_ResultCode->valuestring, "True") == 0){
                        cJSON *json_ResponseData = cJSON_GetObjectItem(json, "ResponseData");
                        if(json_ResponseData){
                            cJSON *json_ResponseData_0 = cJSON_GetArrayItem(json_ResponseData, 0);
                            if(json_ResponseData_0){
                                cJSON *json_PicPath = cJSON_GetObjectItem(json_ResponseData_0, "PicPath");
                                if(json_PicPath && json_PicPath->type == cJSON_String){
                                    pic_url = string(json_PicPath->valuestring);
                                    ret = 0;
                                }
                            }
                        }
                    }
                }
                // 释放内存空间
                cJSON_Delete(json);
            }
            if(ret != 0){
                ERR("data_back[%lu]:%s", data_back.size(), data_back.data());
            }
        }else{
            ERR("http_post_filelds() ERR.");
            //local_dns_resolve_set_err(ipaddr.c_str());  /*设置hosterr*/
        }
    }

    return ret;
}

static void TraverseDir(const char *path, std::vector<std::string> &fileList)
{
    struct dirent *ent = NULL;
    DIR *pDir;
    char childPath[512] = {0};

    if((pDir = opendir(path)) != NULL)
    {
        while(NULL != (ent = readdir(pDir)))
        {
            snprintf(childPath,sizeof(childPath)-1,"%s/%s",path,ent->d_name);
            //   DBG("%s %d", ent->d_name, ent->d_type);
            //   if(ent->d_type == 8){     // d_type：8-文件，4-目录
            if(1){
                int size = strlen(ent->d_name);
                //如果是.jpg文件，长度至少是5
                if(size<5)
                    continue;
                if(strcmp( ( ent->d_name + (size - 4) ) , ".jpg") != 0 && strcmp( ( ent->d_name + (size - 4) ) , ".jpg") != 0)
                    continue;
                //DBG("File:\t%s\n", childPath);
                fileList.push_back(std::string(childPath));
            }
            else if(ent->d_name[0] != '.')
            {
                DBG("\n[Dir]:\t%s\n", childPath);
                TraverseDir(childPath, fileList);     // 递归遍历子目录
                DBG("返回[%s]\n\n", childPath);
            }
        }
        closedir(pDir);
    }
    else
        DBG("Open Dir-[%s] failed.\n", path);
}

static void get_one_pic(const char* camera_ip, char* pic_path)
{
    vector<string> fileList;
    fileList.clear();
    char dir_path[128];
    int camera_type = get_camera_type(camera_ip);
    if(camera_type == 1)
    {
        int check_range = get_camera_check_range(camera_ip);
        if(check_range == 2){
            sprintf(dir_path, "/dev/shm/save_track");
        }
        else{
            sprintf(dir_path, "/dev/shm/%s", camera_ip);
        }
    }
    else if(camera_type == 2)
    {
        sprintf(dir_path, "/dev/shm/%s", camera_ip);
    }
    else
    {
        ERR("%s camera_ip[%s] err", __FUNCTION__, camera_ip);
        return;
    }
    
    TraverseDir(dir_path, fileList);
    if(fileList.size()==0)
    {
        return;
    }
    else
    {
        sprintf(pic_path, "%s", fileList[fileList.size()-1].c_str());
    }

    return;
}
/*static int ReadImageFromFile(const char *picPath, cv::Mat &imageMat)
{
    imageMat = cv::imread(picPath);
    if( !imageMat.data ) { 
        
        return -1; 
    }

    return 0;

}*/
/*文件路径用_代替.最后一个.除外*/
static void FilePathFilter(char *filePath)
{
    if(!filePath){
        return;
    }
    int lastDotIndex = -1;
    for(unsigned int i = 0; i <= strlen(filePath); i++){
        if(filePath[i] == '.' || filePath[i] == ':' || filePath[i] == ' ' || filePath[i] == ','){
            if(filePath[i] == '.'){
                lastDotIndex = i;
            }
            filePath[i] = '_';
        }
    }
    if(lastDotIndex != -1){
        filePath[lastDotIndex] = '.';
    }

    return;
}

/*
//从cameraIp相机缓存图片中获取当前时间count秒内的相机图像路径imagePath,并进行指定数据压缩
//返回值：-1：失败； 0： 成功；
int get_camera_compress_image_path(const char *cameraIp, char *cameraImagePath, 
                                            size_t size, unsigned int count, int compressionRatios)
{
    cv::Mat src;
    char image_path[128] = {0};
    memset(image_path, 0, sizeof(image_path));
    get_one_pic(cameraIp, image_path);
    if(strlen(image_path) > 0)
    {
        if(ReadImageFromFile(image_path, src) < 0 ) { 
            DBG("ReadImageFromFile failed.");
            return -1; 
        }
        snprintf(cameraImagePath, size, "eoc_get_%s.jpg", cameraIp);
        FilePathFilter(cameraImagePath);

        vector<int>compression_params(2);
        compression_params[0] = IMWRITE_JPEG_QUALITY;        
        compression_params[1] = compressionRatios;
 
        imwrite(cameraImagePath,src,compression_params);
        return 0;
    }

    return -1;
}
*/


//上传图片到文件服务器
//返回值1,上传成功；0,上传失败
//int upload_img_get_url(char *ip, int compress_ratio, int type, string &url)
//{
//    int ret = 0;
//    int send_pic_flag = 0;
//    if(ip == NULL)
//    {
//        ERR("get pic camera ip=NULL");
//        return -1;
//    }
//    DBG("get image Type = %d", type);
//    char local_img_path[128] = {0};
///*
//    if(img_type==3){    //白3（非实时图，白天不带车牌）
//        sprintf(specific_img_path, "/dev/shm/%s_3.jpg", cameraIP);
//        FilePathFilter(specific_img_path);
//        if(access(specific_img_path, R_OK|R_OK) != 0)
//        {
//            DBG("%s pic null", specific_img_path);
//            ret = -1;
//        //  memset(img_path, 0, sizeof(img_path));
//        //  ret = get_camera_compress_image_path(cameraIP, img_path, sizeof(img_path), 20, 100);
//        }
//        else
//        {
//            ret = save_pic_by_compress(specific_img_path, img_path, compressionRatios);
//        }
//    }else if(img_type==5){  //黑2（非实时图，夜间，带车牌）
//        sprintf(specific_img_path, "/dev/shm/%s_5.jpg", cameraIP);
//        FilePathFilter(specific_img_path);
//        if(access(specific_img_path, R_OK|R_OK) != 0)
//        {
//            DBG("%s pic null", specific_img_path);
//            ret = -1;
//        //  memset(img_path, 0, sizeof(img_path));
//        //  ret = get_camera_compress_image_path(cameraIP, img_path, sizeof(img_path), 20, 100);
//        }
//        else
//        {
//            ret = save_pic_by_compress(specific_img_path, img_path, compressionRatios);
//        }
//    }else{  //0,1,2,4 实时图片
//*/
//        ret = get_camera_compress_image_path(ip, local_img_path, sizeof(local_img_path), 20, compress_ratio);
////    }
//    if(ret == 0 && strlen(local_img_path) > 0){
//        ret = send_picture_to_fileserver(local_img_path, url);
//        if(ret == 0){
//            DBG("pic_url = %s", url.c_str());
//            send_pic_flag = 1;
//        }else{
//            ERR("%s:send picture[%s] to fileserver ERR", __FUNCTION__, local_img_path);
//            send_pic_flag = 0;
//        }
//        //删除图片
//   //     if (remove(local_img_path) != 0) {
//   //         ERR("remove %s error", local_img_path);
//   //     }
//    }else{
//        send_pic_flag = 0;
//        ERR("%s:get picture from camera[%s],ret=%d ERR", __FUNCTION__, ip, ret);
//    }
//
//    return send_pic_flag;
//}

static int compute_file_md5(const char *file_path, const char *md5_check)
{
    int fd;
    int ret;
    unsigned char data[1024];
    unsigned char md5_value[16];
    char md5_tmp[64] = {0};
    MD5_CTX md5;

    fd = open(file_path, O_RDONLY);
    if (-1 == fd){
        ERR("%s:open file[%s] Err", __FUNCTION__, file_path);
        return -1;
    }
    MD5Init(&md5);
    while (1){
        ret = read(fd, data, 1024);
        if (-1 == ret){
            ERR("%s:read file[%s] Err", __FUNCTION__, file_path);
            return -1;
        }
        MD5Update(&md5, data, ret);
        if(0 == ret || ret < 1024){
            break;
        }
    }
    close(fd);
    MD5Final(&md5, md5_value);

    for(int i=0; i<16; i++){
        snprintf(md5_tmp + i*2, 2+1, "%02x", md5_value[i]);
    }
    
    if(strcmp(md5_check, md5_tmp) == 0){
        return 0;
    }else{
        INFO("%s md5check failed:file_md5=%s,check_md5=%s", __FUNCTION__, md5_tmp, md5_check);
    }

    return -1;
}
static ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;
    ptr = (const char *)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;        /* and call write() again */
            else
                return(-1);            /* error */
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}
static size_t download_file_callback(void *buffer, size_t size, size_t nmemb, void *userp) {
    static int count = 0;
    if(count++%20 == 0){
        printf(".");
        fflush(stdout);
    }
    return writen(*(int *)userp, buffer, nmemb*size);
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
static int eoc_download_file(const char *url, int timeout_ms, const char* file_name,
        int file_size, const char* file_md5)
{
    DBG("%s:url=%s,timeout_ms=%d,file_name=%s,file_size=%d,file_md5=%s", __FUNCTION__,
            url, timeout_ms, file_name, file_size, file_md5);
    int ret = 0;
    CURL *curl;
    CURLcode res;
    vector<char> file_buff;
    struct curl_slist *headerlist=NULL;

    int fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if(fd <= 0){
        ERR("%s:create file:%s Err", __FUNCTION__, file_name);
        return -3;
    }
    std::string ipurl;
    std::string host,port;
    search_DNS_ipurl(url,ipurl,0,host,port);
    host = "Host:" + host;
    port = "Port:" + port;
    headerlist = curl_slist_append(headerlist, "Expect:");
    //headerlist = curl_slist_append(headerlist, "Accept: application/json");
    headerlist = curl_slist_append(headerlist, host.c_str());
    headerlist = curl_slist_append(headerlist, port.c_str());

    curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms );            //接收数据时超时设置，如果10秒内数据未接收完，直接退出
        //curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);                       //查找次数，防止查找太深
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);      //连接超时，这个数值如果设置太短可能导致数据请求不到就断开了
        curl_easy_setopt(curl, CURLOPT_URL, (char *)ipurl.c_str());//请求的URL
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
        if(res == CURLE_OK && http_response_code == 200){
            //请求成功
            DBG("download_file[%s] to %s succeed", url, file_name);
            ret = 0;
            if(compute_file_md5(file_name, file_md5) != 0)
            {
                ERR("%s:%s MD5校验失败", __FUNCTION__, file_name);
                ret = -4;
            }
        }else if(res == CURLE_OPERATION_TIMEDOUT){
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -2;
        }else if(res == CURLE_WRITE_ERROR){
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -3;
        }else{
            ERR("download_file[%s] to %s: failed: %s", url, file_name, curl_easy_strerror(res));
            ret = -1;
        }
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(headerlist);
    close(fd);
    return ret;
}

static void* eoc_download_thread(void *argv)
{
    DBG("%s:Enter.argv:%s", __FUNCTION__, (char *)argv);
    int ret = 0;
    char *download_url = (char *)argv;
    char update_file_name[256] = {0};
    size_t tmp_i = 0;
    for(tmp_i=0; tmp_i<eoc_comm_data.downloads_msg.size(); tmp_i++)
    {
        DBG("[%ld]url:%s", tmp_i, eoc_comm_data.downloads_msg[tmp_i].download_url.c_str());
        if(strcmp(eoc_comm_data.downloads_msg[tmp_i].download_url.c_str(), download_url) == 0)
        {
            //找到下载链接信息
            if(eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size() == 0)
            {
                //主控板升级下载文件
                snprintf(update_file_name, 256, "%s", UPDATEFILE);
            }
            else
            {
                if(eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[0].dev_type == EOC_UPGRADE_PARKINGAREA)
                {
                    //主控板升级下载文件
                    snprintf(update_file_name, 256, "%s", UPDATEFILE);
                }
                else
                {
                    //相机或矩阵控制器升级下载文件
                    snprintf(update_file_name, 256, "%s_%s", 
                        eoc_comm_data.downloads_msg[tmp_i].download_file_md5.c_str(), 
                        eoc_comm_data.downloads_msg[tmp_i].download_file_name.c_str());
                }
            }
            ret = eoc_download_file(eoc_comm_data.downloads_msg[tmp_i].download_url.c_str(),
                                    8*60*1000,
                                    update_file_name,
                                    eoc_comm_data.downloads_msg[tmp_i].download_file_size,
                                    eoc_comm_data.downloads_msg[tmp_i].download_file_md5.c_str());
            if(ret == 0){
                DBG("%s:下载文件完成", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_FINISHED;
            }else if(ret == -1){
                ERR("%s:下载文件不存在", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_FILE_NOT_EXIST;
            }else if(ret == -2){
                ERR("%s:下载超时", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_TIMEOUT;
            }else if(ret == -3){
                ERR("%s:本地剩余空间不足", __FUNCTION__);
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_SPACE_NOT_ENOUGH;
            }else if(ret == -4){
                eoc_comm_data.downloads_msg[tmp_i].download_status = DOWNLOAD_MD5_FAILD;
            }
            break;
        }
    }

    if(tmp_i == eoc_comm_data.downloads_msg.size())
    {
        DBG("serch download_url err");
    }

    DBG("%s:Exit.", __FUNCTION__);
    return NULL;
}


bool is_login = false;
int heart_flag = 0;
static string db_conf_version;
static string eoc_rec_data; //接收到的数据

#define EOC_COMM_RECEIVE_100 "EAGLER100"    //心跳
#define EOC_COMM_SEND_100    "EAGLES100"
#define EOC_COMM_RECEIVE_101 "EAGLER101"    //登录
#define EOC_COMM_SEND_101    "EAGLES101"
#define EOC_COMM_RECEIVE_102 "EAGLER102"    //配置下发
#define EOC_COMM_SEND_102    "EAGLES102"
#define EOC_COMM_RECEIVE_103 "EAGLER103"    //状态上传
#define EOC_COMM_SEND_103    "EAGLES103"
#define EOC_COMM_RECEIVE_104 "EAGLER104"    //实时图片开始获取
#define EOC_COMM_SEND_104    "EAGLES104"
#define EOC_COMM_RECEIVE_105 "EAGLER105"    //实时图片停止获取
#define EOC_COMM_SEND_105    "EAGLES105"
#define EOC_COMM_RECEIVE_107 "EAGLER107"    //配置获取
#define EOC_COMM_SEND_107    "EAGLES107"
#define EOC_COMM_RECEIVE_108 "EAGLER108"    //外网状态上传
#define EOC_COMM_SEND_108    "EAGLES108"
#define EOC_COMM_RECEIVE_109 "EAGLER109"    //相机软重启
#define EOC_COMM_SEND_109    "EAGLES109"
#define EOC_COMM_RECEIVE_110 "EAGLER110"    //雷视机软件下载
#define EOC_COMM_SEND_110    "EAGLES110"
#define EOC_COMM_RECEIVE_111 "EAGLER111"    //雷视机软件升级
#define EOC_COMM_SEND_111    "EAGLES111"

#define EOC_COMM_CAMERA_R002 "CameraR002"   //相机详细设置下发
#define EOC_COMM_CAMERA_S002 "CameraS002"
#define EOC_COMM_CAMERA_R004 "CameraR004"   //相机获取图片
#define EOC_COMM_CAMERA_S004 "CameraS004"
#define EOC_COMM_CAMERA_R006 "CameraR006"   //相机软件下载
#define EOC_COMM_CAMERA_S006 "CameraS006"
#define EOC_COMM_CAMERA_R007 "CameraR007"   //相机软件升级
#define EOC_COMM_CAMERA_S007 "CameraS007"


//相机下载、升级
int s110_download_send(tcp_client_t* client, const char *comm_guid, 
                            int result, int stats, int progress, const char *msg);
//相机下载、升级
int sc006_download_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg);


//添加下载任务
//返回值：0添加成功；-1添加失败; 1正在升级退出
int add_eoc_download_event(EocDownloadsMsg &data, EocUpgradeDev &dev_data)
{
    int ret = 0;

    size_t tmp_i = 0;
    size_t tmp_j = 0;
    for(tmp_i=0; tmp_i<eoc_comm_data.downloads_msg.size(); tmp_i++)
    {
        if(data.download_url == eoc_comm_data.downloads_msg[tmp_i].download_url)
        {
            //下载链接已存在
            DBG("%s:download_thread is running, add ip %s", __FUNCTION__, dev_data.dev_ip.c_str());
            if(dev_data.dev_type == EOC_UPGRADE_PARKINGAREA)
            {
                DBG("雷视机升级下载已添加");
                return 1;
            }
            for(tmp_j=0; tmp_j<eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size(); tmp_j++)
            {
                if((eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[tmp_j].dev_ip == dev_data.dev_ip)
                    &&(eoc_comm_data.downloads_msg[tmp_i].upgrade_dev[tmp_j].dev_guid == dev_data.dev_guid))
                {
                    DBG("固件下载已添加");
                    return 1;
                }
            }
            if(tmp_j == eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.size())
            {
                DBG("固件下载添加");
                eoc_comm_data.downloads_msg[tmp_i].upgrade_dev.push_back(dev_data);
            }
        //    eoc_comm_data.downloads_msg[tmp_i].comm_guid = data.comm_guid;
            ret = 0;
            break;
        }
    }
    if(tmp_i == eoc_comm_data.downloads_msg.size())
    {
        //下载链接不存在，添加下载
        data.download_status = DOWNLOAD_IDLE;
        data.upgrade_dev.clear();
        data.upgrade_dev.push_back(dev_data);
        eoc_comm_data.downloads_msg.push_back(data);
        ret = pthread_create(&eoc_comm_data.downloads_msg[tmp_i].download_thread_id, NULL, 
                            eoc_download_thread, (void *)eoc_comm_data.downloads_msg[tmp_i].download_url.c_str());
        if(ret != 0){
            ERR("%s:create download_thread Err", __FUNCTION__);
            return -1;
        }
    }

    return 0;
}
//添加升级任务
//返回值：0添加成功；-1添加失败; 1升级任务已存在
int add_eoc_upgrade_event(EocUpgradeDev &data)
{
    int ret = 0;
    //判断此guid设备是否在升级队列，不在添加
    size_t tmp_i = 0;
    for(tmp_i=0; tmp_i<eoc_comm_data.upgrade_msg.size(); tmp_i++)
    {
        if(eoc_comm_data.upgrade_msg[tmp_i].dev_guid == data.dev_guid)
        {
            DBG("升级guid已存在, ip:%s", data.dev_ip.c_str());
            return 1;
        }
    }
    if(tmp_i == eoc_comm_data.upgrade_msg.size())
    {
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
//获取相机升级任务
//返回值：0无相机升级任务，>0需升级相机数


//添加自动升级任务
static int start_auto_upgrade()
{
    unsigned int tmp_i = 0;
    for(tmp_i=0; tmp_i<eoc_comm_data.task.size(); tmp_i++)
    {
        if(eoc_comm_data.task[tmp_i] == AUTO_UPGRADE)
        {
            DBG("自动升级任务已存在");
            break;
        }
    }
    if(tmp_i == eoc_comm_data.task.size())
    {
        eoc_comm_data.task.push_back(AUTO_UPGRADE);
    }
    return 0;
}
//添加系统重启任务
static int add_reboot_task()
{
    unsigned int tmp_i = 0;
    if((eoc_comm_data.task.size()==0) && (eoc_comm_data.downloads_msg.size()==0) && (eoc_comm_data.upgrade_msg.size()==0))
    {
        DBG("System is going to reboot!");
        sleep(5);
        _exit(0);
    }
    for(tmp_i=0; tmp_i<eoc_comm_data.task.size(); tmp_i++)
    {
        if(eoc_comm_data.task[tmp_i] == SYS_REBOOT)
        {
            DBG("进程重启任务已存在");
            break;
        }
    }
    if(tmp_i == eoc_comm_data.task.size())
    {
        eoc_comm_data.task.push_back(SYS_REBOOT);
    }
    return 0;
}

//向EOC返回下载结果
static int send_download_msg(tcp_client_t* client, EocDownloadsMsg *downloade_msg)
{
    int ret = 0;
#if 0
    if(downloade_msg->upgrade_dev.size() == 0)
    {
        //主控板下载
        if(downloade_msg->download_status == DOWNLOAD_FINISHED){
            ret = s110_download_send(client, downloade_msg->comm_guid.c_str(), 2, 1, 0, "下载完成 MD5 OK");
        }else if(downloade_msg->download_status == DOWNLOAD_FILE_NOT_EXIST){
            ret = s110_download_send(client, downloade_msg->comm_guid.c_str(), 2, 0, 0, "下载文件不存在");
        }else if(downloade_msg->download_status == DOWNLOAD_TIMEOUT){
            ret = s110_download_send(client, downloade_msg->comm_guid.c_str(), 2, 0, 0, "下载超时");
        }else if(downloade_msg->download_status == DOWNLOAD_SPACE_NOT_ENOUGH){
            ret = s110_download_send(client, downloade_msg->comm_guid.c_str(), 2, 0, 0, "本地剩余空间不足");
        }else if(downloade_msg->download_status == DOWNLOAD_MD5_FAILD){
            ret = s110_download_send(client, downloade_msg->comm_guid.c_str(), 2, 0, 0, "MD5校验失败");
        }
        DBG("主控板下载");
        return ret;
    }
#endif
    for(size_t i=0; i<downloade_msg->upgrade_dev.size(); i++)
    {
        if(downloade_msg->upgrade_dev[i].dev_type == EOC_UPGRADE_PARKINGAREA)
        {
            //雷视机升级
            if(downloade_msg->download_status == DOWNLOAD_FINISHED){
                ret = s110_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 1, 0, "下载完成 MD5 OK");
            }else if(downloade_msg->download_status == DOWNLOAD_FILE_NOT_EXIST){
                ret = s110_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0, "下载文件不存在");
            }else if(downloade_msg->download_status == DOWNLOAD_TIMEOUT){
                ret = s110_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0, "下载超时");
            }else if(downloade_msg->download_status == DOWNLOAD_SPACE_NOT_ENOUGH){
                ret = s110_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0, "本地剩余空间不足");
            }else if(downloade_msg->download_status == DOWNLOAD_MD5_FAILD){
                ret = s110_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), 2, 0, 0, "MD5校验失败");
            }
        }
        else if(downloade_msg->upgrade_dev[i].dev_type == EOC_UPGRADE_CAMARA)
        {
            //相机固件下载
            if(downloade_msg->download_status == DOWNLOAD_FINISHED){
                ret = sc006_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 1, 0, "下载完成 MD5 OK");
            }else if(downloade_msg->download_status == DOWNLOAD_FILE_NOT_EXIST){
                ret = sc006_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "下载文件不存在");
            }else if(downloade_msg->download_status == DOWNLOAD_TIMEOUT){
                ret = sc006_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "下载超时");
            }else if(downloade_msg->download_status == DOWNLOAD_SPACE_NOT_ENOUGH){
                ret = sc006_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "本地剩余空间不足");
            }else if(downloade_msg->download_status == DOWNLOAD_MD5_FAILD){
                ret = sc006_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "MD5校验失败");
            }
        //    DBG("相机固件下载");
        }
        else if(downloade_msg->upgrade_dev[i].dev_type == EOC_UPGRADE_CONTROLLER)
        {
            //矩阵控制器文件下载
            if(downloade_msg->download_status == DOWNLOAD_FINISHED){
                ret = cks008_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 1, 0, "下载完成 MD5 OK");
            }else if(downloade_msg->download_status == DOWNLOAD_FILE_NOT_EXIST){
                ret = cks008_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "下载文件不存在");
            }else if(downloade_msg->download_status == DOWNLOAD_TIMEOUT){
                ret = cks008_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "下载超时");
            }else if(downloade_msg->download_status == DOWNLOAD_SPACE_NOT_ENOUGH){
                ret = cks008_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "本地剩余空间不足");
            }else if(downloade_msg->download_status == DOWNLOAD_MD5_FAILD){
                ret = cks008_download_send(client, downloade_msg->upgrade_dev[i].comm_guid.c_str(), downloade_msg->upgrade_dev[i].dev_guid.c_str(), 
                                                    2, 0, 0, "MD5校验失败");
            }
        }

        if(ret != 0)
            break;
    }

    return ret;
}


static int get_json_int(cJSON *j_data, string str)
{
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get int %s err", str.c_str());
        return 0;
    }
    else
    {
        return cJSON_GetObjectItem(j_data, str.c_str())->valueint;
    }
}
static double get_json_double(cJSON *j_data, string str)
{
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get int %s err", str.c_str());
        return 0;
    }
    else
    {
        return cJSON_GetObjectItem(j_data, str.c_str())->valuedouble;
    }
}
static string get_json_string(cJSON *j_data, string str)
{
    string get_str;
    if(cJSON_GetObjectItem(j_data, str.c_str()) == NULL)
    {
        ERR("json get string %s err", str.c_str());
        get_str = "";
        return get_str;
    }
    else
    {
        if(cJSON_GetObjectItem(j_data, str.c_str())->valuestring != NULL)
        {
            get_str = cJSON_GetObjectItem(j_data, str.c_str())->valuestring;
        }
        else
        {
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
static int s100_heart_send(tcp_client_t* client)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_100);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_100);

    json_str=cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
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
static int r100_heart_deal(cJSON *json)
{
    int ret = 0;
    char code[16] = {0};
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    sprintf(code, "%s", get_json_string(data_json,"Code").c_str());

    if(strcmp(code, EOC_COMM_RECEIVE_100) == 0){
        heart_flag = 0;
        DBG("receive EARLER100");
    }else{
        ERR("rec 100 Data-Code err : %s", code);
    }

    return 0;
}

/*
 * 登录eoc
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s101_login_send(tcp_client_t* client)
{
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
    if(ret != 0){
        ERR("%s:get_factory_uname() Err", __FUNCTION__);
        return -1;
    }
//    equip_num = "aab65d77-8242-4f31-b088-46eb038663d9";

    getipaddr(ethip,n2nip);
    sprintf(ipmsg, "%s[%s]", ethip, n2nip);

    string db_data_version;
    db_version_get(db_data_version);

    //打包发送信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        DBG("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
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
    cJSON_AddStringToObject(data, "EquipType", "NX");
    cJSON_AddStringToObject(data, "SoftVersion", "V1.1.1");
    cJSON_AddStringToObject(data, "CacheDataVersion", db_data_version.c_str());

    json_str = cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";
    DBG("%s:%s", __FUNCTION__, send_str.c_str());
    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);

    return 0;
}

static int r101_login_deal(cJSON *json)
{
    int ret = 0;

    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    
    if(get_json_int(data_json,"State") == 1){
        DBG("101登录完成");
        is_login = true;
        ret = 0;
    }else{
        ERR("101登录失败：%s", get_json_string(data_json,"Message").c_str());//读取错误信息
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
static int s102_config_download_send(tcp_client_t* client, const char *comm_guid, int state, const char *msg)
{
    if(strlen(comm_guid)<=0)
    {
        DBG("comm_guid null");
        return 0;
    }

    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
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

    json_str=cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";
    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
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
static int update_conf_version(const char* version)
{
    //更新数据库版本号
    int ret = 0;
    DB_Conf_Data_Version data;
    time_t now;
    struct tm timenow;
    time(&now);
    localtime_r(&now, &timenow);
    char time_p[50] = {0};
    sprintf(time_p, "%u-%02u-%02u %02u:%02u:%02u",
            1900+timenow.tm_year, 1+timenow.tm_mon, timenow.tm_mday,
            timenow.tm_hour, timenow.tm_min, timenow.tm_sec);
    DBG("write to database,dbversion = %s, time_p = %s", version, time_p);

    ret = db_version_delete();
    if(ret != 0){
        ERR("%s:db_delete_version() Err", __FUNCTION__);
        return -1;
    }

    data.Id = 0;
    data.version = version;
    data.time = time_p;

    ret = db_version_insert(data);
    if(ret != 0){
        ERR("%s:db_insert_version() Err", __FUNCTION__);
        return -1;
    }

    return 0;
}
//车道区域信息解析
int analyze_lane_area_infos(cJSON *lane_area_array_json, string &camera_guid)
{
    if(lane_area_array_json == NULL)
    {
        ERR("lane_area_array_json null");
        return -1;
    }

    int lane_area_array_size = cJSON_GetArraySize(lane_area_array_json);
    for(int i=0; i<lane_area_array_size; i++)
    {
        cJSON *lane_area_json = cJSON_GetArrayItem(lane_area_array_json, i);
        if(lane_area_json == NULL)
        {
            ERR("%s:get object item LaneAreaInfo NULL Err", __FUNCTION__);
            return -1;
        }
        string lane_guid = get_json_string(lane_area_json, "LaneGuid");
        //车道线
        cJSON *lane_line_array_json = cJSON_GetObjectItem(lane_area_json, "LaneLineInfos");
        if(lane_line_array_json != NULL)
        {
            int lane_line_array_size = cJSON_GetArraySize(lane_line_array_json);
            for(int j=0; j<lane_line_array_size; j++)
            {
                cJSON *lane_line_json = cJSON_GetArrayItem(lane_line_array_json, j);
                if(lane_line_json == NULL)
                {
                    ERR("%s:get object item LaneLineInfos[%d] NULL Err", __FUNCTION__, j);
                    break;
                }
                DB_Lane_Line_T lane_line;
                lane_line.LaneGuid = lane_guid;
                lane_line.LaneLineGuid = get_json_string(lane_line_json, "LaneLineGuid");
                lane_line.Type = get_json_int(lane_line_json, "Type");
                lane_line.Color = get_json_int(lane_line_json, "Color");
                lane_line.CoordinateSet = get_json_string(lane_line_json, "CoordinateSet");
                lane_line.CameraGuid = camera_guid;
                db_lane_line_insert(lane_line);
            }
        }
        else
        {
            ERR("%s:get object item LaneLineInfos NULL Err", __FUNCTION__);
        }
        //识别区
        cJSON *identif_area_array_json = cJSON_GetObjectItem(lane_area_json, "IdentificationAreas");
        if(identif_area_array_json != NULL)
        {
            
            int identif_area_array_size = cJSON_GetArraySize(identif_area_array_json);
            for(int j=0; j<identif_area_array_size; j++)
            {
                cJSON *identif_area_json = cJSON_GetArrayItem(identif_area_array_json, j);
                if(identif_area_json == NULL)
                {
                    ERR("%s:get object item IdentificationAreas[%d] NULL Err", __FUNCTION__, j);
                    break;
                }
                DB_Lane_Identif_Area_T identif_area;
                identif_area.LaneGuid = lane_guid;
                identif_area.IdentifAreaGuid = get_json_string(identif_area_json, "Guid");
                identif_area.Type = get_json_int(identif_area_json, "Type");
                identif_area.CameraGuid = camera_guid;
                db_lane_identif_area_insert(identif_area);
                cJSON *identif_area_p_array_json = cJSON_GetObjectItem(identif_area_json, "IdentificationAreaPoints");
                if(identif_area_p_array_json != NULL)
                {
                    
                    int identif_area_p_array_size = cJSON_GetArraySize(identif_area_p_array_json);
                    for(int m=0; m<identif_area_p_array_size; m++)
                    {
                        cJSON *identif_area_p_json = cJSON_GetArrayItem(identif_area_p_array_json, m);
                        if(identif_area_p_json == NULL)
                        {
                            ERR("%s:get object item IdentificationAreaPoints[%d] NULL Err", __FUNCTION__, m);
                            break;
                        }
                        DB_Lane_Identif_Area_Point_T identif_point;
                        identif_point.IdentifAreaGuid = identif_area.IdentifAreaGuid;
                        identif_point.SerialNumber = get_json_int(identif_area_p_json, "SerialNumber");
                        identif_point.XPixel = get_json_double(identif_area_p_json, "XPixel");
                        identif_point.YPixel = get_json_double(identif_area_p_json, "YPixel");
                        db_lane_identif_point_insert(identif_point);
                    }
                }
                else
                {
                    ERR("%s:get object item IdentificationAreas[%d]IdentificationAreaPoints NULL Err", __FUNCTION__, j);
                    break;
                }
            }
        }
        else
        {
            ERR("%s:get object item IdentificationAreas NULL Err", __FUNCTION__);
        }
                
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
static int r102_config_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    DB_Base_Set_T base_set_db;
    DB_Intersection_t intersection_db;
    DB_Camera_Info_T camera_info_db;
    DB_Radar_Info_T radar_info_db;
/*
    if(strlen(comm_guid) <= 0)
    {
        DBG("102 deal fail, comm_guid NULL");
        return 0;
    }
*/
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    if(data_json == NULL)
    {
        ERR("%s:get object item Data NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item Data NULL");
        return ret;
    }
    base_set_db.Index = get_json_int(data_json, "Index");
    //核心板基础配置
    cJSON *base_set_json = cJSON_GetObjectItem(data_json, "BaseSetting");
    if(base_set_json == NULL)
    {
        ERR("%s:get object item BaseSetting NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item BaseSetting NULL");
        return ret;
    }
    base_set_db.City = get_json_string(base_set_json, "City");
    base_set_db.IsUploadToPlatform = get_json_int(base_set_json, "IsUploadToPlatform");
    base_set_db.Is4Gmodel = get_json_int(base_set_json, "Is4Gmodel");
    base_set_db.IsIllegalCapture = get_json_int(base_set_json, "IsIllegalCapture");
    base_set_db.PlateDefault = get_json_string(base_set_json, "PlateDefault");
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
    cJSON *cck1_json = cJSON_GetObjectItem(data_json, "CCK1Info");
    base_set_db.CCK1Guid = get_json_string(cck1_json, "CCK1Guid");
    base_set_db.CCK1IP = get_json_string(cck1_json, "CCK1IP");
    base_set_db.ErvPlatId = get_json_string(data_json, "ErvPlatId");
    db_base_set_delete();
    db_base_set_insert(base_set_db);

    //所属路口信息
    cJSON *intersection_info_json = cJSON_GetObjectItem(data_json, "IntersectionInfo");
    if(intersection_info_json == NULL)
    {
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
    if(intersect_set_json == NULL)
    {
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
    }
    else
    {
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

    //相机设置信息
    cJSON *camera_array_json = cJSON_GetObjectItem(data_json, "CameraInfos");
    if(camera_array_json == NULL)
    {
        ERR("%s:get object item CameraInfos NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item CameraInfos NULL");
        return ret;
    }
    int camera_array_size = cJSON_GetArraySize(camera_array_json);
    db_camera_info_delete();
    for(int i=0; i<camera_array_size; i++)
    {
        cJSON *camera_info_json = cJSON_GetArrayItem(camera_array_json, i);
        if(camera_info_json == NULL)
        {
            ERR("%s:get object item CameraInfos NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item CameraInfos NULL");
            return ret;
        }
        camera_info_db.Guid = get_json_string(camera_info_json, "Guid");
        camera_info_db.Ip = get_json_string(camera_info_json, "Ip");
        camera_info_db.Name = get_json_string(camera_info_json, "CameraName");
        camera_info_db.AccessMode = get_json_int(camera_info_json, "AccessMode");
        camera_info_db.Type = get_json_int(camera_info_json, "Type");
        camera_info_db.CheckRange = get_json_int(camera_info_json, "CheckRange");
        camera_info_db.Direction = get_json_int(camera_info_json, "Direction");
        camera_info_db.PixelType = get_json_int(camera_info_json, "PixelType");
        camera_info_db.Delay = get_json_int(camera_info_json, "Delay");
        camera_info_db.IsEnable = get_json_int(camera_info_json, "IsEnable");
        db_camera_info_insert(camera_info_db);
    }
    
    //雷达设置信息
    cJSON *radar_info_json = cJSON_GetObjectItem(data_json, "RadarInfo");
    if(radar_info_json == NULL)
    {
        ERR("%s:get object item RadarInfo NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item RadarInfo NULL");
        return ret;
    }
    radar_info_db.Guid = get_json_string(radar_info_json, "Guid");
    radar_info_db.Name = get_json_string(radar_info_json, "Name");
    radar_info_db.Ip = get_json_string(radar_info_json, "Ip");
    radar_info_db.Port = get_json_int(radar_info_json, "Port");
    radar_info_db.XThreshold = get_json_double(radar_info_json, "XThreshold");
    radar_info_db.YThreshold = get_json_double(radar_info_json, "YThreshold");
    radar_info_db.XCoordinate = get_json_double(radar_info_json, "XCoordinate");
    radar_info_db.YCoordinate = get_json_double(radar_info_json, "YCoordinate");
    radar_info_db.XRadarCoordinate = get_json_double(radar_info_json, "XRadarCoordinate");
    radar_info_db.YRadarCoordinate = get_json_double(radar_info_json, "YRadarCoordinate");
    radar_info_db.HRadarCoordinate = get_json_double(radar_info_json, "HRadarCoordinate");
    radar_info_db.CorrectiveAngle = get_json_double(radar_info_json, "CorrectiveAngle");
    radar_info_db.XOffset = get_json_double(radar_info_json, "XOffset");
    radar_info_db.YOffset = get_json_double(radar_info_json, "YOffset");
    radar_info_db.CheckDirection = get_json_int(radar_info_json, "CheckDirection");
    db_radar_info_delete();
    db_radar_info_insert(radar_info_db);

    //车道信息LaneInfos
    cJSON *lane_array_json = cJSON_GetObjectItem(data_json, "LaneInfos");
    if(lane_array_json == NULL)
    {
        ERR("%s:get object item LaneInfos NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item LaneInfos NULL");
        return ret;
    }
    int lane_array_size = cJSON_GetArraySize(lane_array_json);
    db_lane_info_delete();
    for(int i=0; i<lane_array_size; i++)
    {
        cJSON *lane_info_json = cJSON_GetArrayItem(lane_array_json, i);
        if(lane_info_json == NULL)
        {
            ERR("%s:get object item LaneInfos NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item LaneInfos NULL");
            return ret;
        }
        DB_Lane_Info_T lane_info;
        lane_info.Guid = get_json_string(lane_info_json, "Guid");
        lane_info.Name = get_json_string(lane_info_json, "Name");
        lane_info.SerialNumber = get_json_int(lane_info_json, "SerialNumber");
        lane_info.DrivingDirection = get_json_int(lane_info_json, "DrivingDirection");
        lane_info.Type = get_json_int(lane_info_json, "Type");
        lane_info.Length = get_json_double(lane_info_json, "Length");
        lane_info.Width = get_json_double(lane_info_json, "Width");
        lane_info.IsEnable = get_json_int(lane_info_json, "IsEnable");
        lane_info.CameraName = get_json_string(lane_info_json, "CameraName");
        db_lane_info_insert(lane_info);
    }

    //区域信息AreaInfo
    cJSON *area_info_json = cJSON_GetObjectItem(data_json, "AreaInfo");
    if(area_info_json == NULL)
    {
        ERR("%s:get object item AreaInfo NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item AreaInfo NULL");
        return ret;
    }
    db_lane_line_delete();  //车道线
    db_lane_identif_area_delete();  //车道识别区
    db_lane_identif_point_delete();  //车道识别区点
    
    //----------检测相机下的区域信息
    cJSON *detect_camera_area_array_json = cJSON_GetObjectItem(area_info_json, "DetectionCameraAreaInfo");
    if(detect_camera_area_array_json == NULL)
    {
        ERR("%s:get object item DetectionCameraAreaInfo NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item DetectionCameraAreaInfo NULL");
        return ret;
    }
    db_detect_shake_area_delete();  //防抖区
    db_detect_shake_point_delete();  //防抖区点
    db_detect_fusion_area_delete();  //融合区
    db_detect_fusion_p_delete();    //融合区点
    
    int array_size = cJSON_GetArraySize(detect_camera_area_array_json);
    for(int detect_camera_i=0; detect_camera_i<array_size; detect_camera_i++)
    {
        cJSON *detect_camera_area_json = cJSON_GetArrayItem(detect_camera_area_array_json, detect_camera_i);
        if(detect_camera_area_json == NULL)
        {
            ERR("%s:get object item DetectionCameraAreaInfo NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item DetectionCameraAreaInfo NULL");
            return ret;
        }
        //-----------=====相机guid
        string camera_guid = get_json_string(detect_camera_area_json, "CameraGuid");
        
        //-----------=====防抖区----->改为[]
        cJSON *shake_area_json = cJSON_GetObjectItem(detect_camera_area_json, "ShakeAreaInfo");
        if(shake_area_json == NULL)
        {
            ERR("%s:get object item ShakeAreaInfo NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item ShakeAreaInfo NULL");
            return ret;
        }
        int shake_area_size = cJSON_GetArraySize(shake_area_json);
        DB_Detect_Shake_Area_T detect_shake_area;
        for(int shake_i=0; shake_i<shake_area_size; shake_i++)
        {
            cJSON *shake_json = cJSON_GetArrayItem(shake_area_json, shake_i);
            if(shake_json == NULL)
            {
                DBG("shake_area_json[%d] getarrayitem err", shake_i);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item ShakeAreaInfo[] NULL");
                return ret;
            }
            detect_shake_area.Guid = get_json_string(shake_json, "Guid");
            detect_shake_area.XOffset = get_json_double(shake_json, "XOffset");
            detect_shake_area.YOffset = get_json_double(shake_json, "YOffset");
            detect_shake_area.CameraGuid = camera_guid;
            db_detect_shake_area_insert(detect_shake_area);
            cJSON *shake_point_array_json = cJSON_GetObjectItem(shake_json, "ShakePoints");
            if(shake_point_array_json == NULL)
            {
                ERR("%s:get object item ShakePoints NULL Err", __FUNCTION__);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item ShakePoints NULL");
                return ret;
            }
            int shake_point_array_size = cJSON_GetArraySize(shake_point_array_json);
            
            for(int i=0; i<shake_point_array_size; i++)
            {
                cJSON *shake_point_json = cJSON_GetArrayItem(shake_point_array_json, i);
                if(shake_point_json == NULL)
                {
                    ERR("%s:get object item ShakePoints NULL Err", __FUNCTION__);
                    ret = s102_config_download_send(client, comm_guid, 0, "get object item ShakePoints NULL");
                    return ret;
                }
                DB_Detect_Shake_Point_T shake_point;
                shake_point.ShakeAreaGuid = detect_shake_area.Guid;
                shake_point.SerialNumber = get_json_int(shake_point_json, "SerialNumber");
                shake_point.XCoordinate = get_json_double(shake_point_json, "XCoordinate");
                shake_point.YCoordinate = get_json_double(shake_point_json, "YCoordinate");
                shake_point.XPixel = get_json_double(shake_point_json, "XPixel");
                shake_point.YPixel = get_json_double(shake_point_json, "YPixel");
                db_detect_shake_point_insert(shake_point);
            }
        }

        //-----------=====融合区----->改为[]
        cJSON *fusion_area_json = cJSON_GetObjectItem(detect_camera_area_json, "FusionAreaInfo");
        if(fusion_area_json == NULL)
        {
            ERR("%s:get object item FusionAreaInfo NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item FusionAreaInfo NULL");
            return ret;
        }
        int fusion_area_size = cJSON_GetArraySize(fusion_area_json);
        DB_Detect_Fusion_Area_T detect_fusion_area;
        for(int fusion_i=0; fusion_i<fusion_area_size; fusion_i++)
        {
            cJSON *fusion_json = cJSON_GetArrayItem(fusion_area_json, fusion_i);
            if(fusion_json == NULL)
            {
                DBG("fusion_area_json[%d] getarrayitem err", fusion_i);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item FusionAreaInfo[] NULL");
                return ret;
            }
            detect_fusion_area.Guid = get_json_string(fusion_json, "Guid");
            detect_fusion_area.XDistance = get_json_double(fusion_json, "XDistance");
            detect_fusion_area.YDistance = get_json_double(fusion_json, "YDistance");
            detect_fusion_area.HDistance = get_json_double(fusion_json, "HDistance");
            detect_fusion_area.MPPW = get_json_double(fusion_json, "MPPW");
            detect_fusion_area.MPPH = get_json_double(fusion_json, "MPPH");
            detect_fusion_area.PerspectiveTransFa00 = 0.0;
            detect_fusion_area.PerspectiveTransFa01 = 0.0;
            detect_fusion_area.PerspectiveTransFa02 = 0.0;
            detect_fusion_area.PerspectiveTransFa10 = 0.0;
            detect_fusion_area.PerspectiveTransFa11 = 0.0;
            detect_fusion_area.PerspectiveTransFa12 = 0.0;
            detect_fusion_area.PerspectiveTransFa20 = 0.0;
            detect_fusion_area.PerspectiveTransFa21 = 0.0;
            detect_fusion_area.PerspectiveTransFa22 = 0.0;
            cJSON *trans_fa_array_json = cJSON_GetObjectItem(fusion_json, "PerspectiveTransFa");
            if(trans_fa_array_json == NULL)
            {
                ERR("%s:get object item PerspectiveTransFa NULL Err", __FUNCTION__);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item PerspectiveTransFa NULL");
                return ret;
            }
            int trans_fa_array_size = cJSON_GetArraySize(trans_fa_array_json);
            if(trans_fa_array_size == 3)
            {
                cJSON *trans_fa0_json = cJSON_GetArrayItem(trans_fa_array_json, 0);
                if((trans_fa0_json != NULL) && (cJSON_GetArraySize(trans_fa0_json) == 3))
                {
                    detect_fusion_area.PerspectiveTransFa00 = cJSON_GetArrayItem(trans_fa0_json, 0)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa01 = cJSON_GetArrayItem(trans_fa0_json, 1)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa02 = cJSON_GetArrayItem(trans_fa0_json, 2)->valuedouble;
                }
                cJSON *trans_fa1_json = cJSON_GetArrayItem(trans_fa_array_json, 1);
                if((trans_fa1_json != NULL) && (cJSON_GetArraySize(trans_fa1_json) == 3))
                {
                    detect_fusion_area.PerspectiveTransFa10 = cJSON_GetArrayItem(trans_fa1_json, 0)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa11 = cJSON_GetArrayItem(trans_fa1_json, 1)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa12 = cJSON_GetArrayItem(trans_fa1_json, 2)->valuedouble;
                }
                cJSON *trans_fa2_json = cJSON_GetArrayItem(trans_fa_array_json, 2);
                if((trans_fa2_json != NULL) && (cJSON_GetArraySize(trans_fa2_json) == 3))
                {
                    detect_fusion_area.PerspectiveTransFa20 = cJSON_GetArrayItem(trans_fa2_json, 0)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa21 = cJSON_GetArrayItem(trans_fa2_json, 1)->valuedouble;
                    detect_fusion_area.PerspectiveTransFa22 = cJSON_GetArrayItem(trans_fa2_json, 2)->valuedouble;
                }
            }
            detect_fusion_area.CameraGuid = camera_guid;
            db_detect_fusion_area_insert(detect_fusion_area);
            
            cJSON *fusion_point_array_json = cJSON_GetObjectItem(fusion_json, "FusionPoints");
            if(fusion_point_array_json == NULL)
            {
                ERR("%s:get object item FusionPoints NULL Err", __FUNCTION__);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item FusionPoints NULL");
                return ret;
            }
            int fusion_point_array_size = cJSON_GetArraySize(fusion_point_array_json);
            
            for(int i=0; i<fusion_point_array_size; i++)
            {
                cJSON *fusion_point_json = cJSON_GetArrayItem(fusion_point_array_json, i);
                if(fusion_point_json == NULL)
                {
                    ERR("%s:get object item FusionPoints NULL Err", __FUNCTION__);
                    ret = s102_config_download_send(client, comm_guid, 0, "get object item FusionPoints NULL");
                    return ret;
                }
                DB_Detect_Fusion_Point_T fusion_point;
                fusion_point.FusionAreaGuid = detect_fusion_area.Guid;
                fusion_point.SerialNumber = get_json_int(fusion_point_json, "SerialNumber");
                fusion_point.XCoordinate = get_json_double(fusion_point_json, "XCoordinate");
                fusion_point.YCoordinate = get_json_double(fusion_point_json, "YCoordinate");
                fusion_point.XPixel = get_json_double(fusion_point_json, "XPixel");
                fusion_point.YPixel = get_json_double(fusion_point_json, "YPixel");
                db_detect_fusion_p_insert(fusion_point);
            }
        }

        //-----------=====车道区域信息
        cJSON *lane_area_array_json = cJSON_GetObjectItem(detect_camera_area_json, "LaneAreaInfos");
        if(lane_area_array_json == NULL)
        {
            ERR("%s:get object item LaneAreaInfos NULL Err", __FUNCTION__);
            ret = s102_config_download_send(client, comm_guid, 0, "get object item LaneAreaInfos NULL");
            return ret;
        }
        else
        {
            analyze_lane_area_infos(lane_area_array_json, camera_guid);
        }
    }
    //----------识别相机下的区域信息
    cJSON *identif_camera_area_json = cJSON_GetObjectItem(area_info_json, "IdentificationCameraAreaInfo");
    if(identif_camera_area_json == NULL)
    {
        ERR("%s:get object item IdentificationCameraAreaInfo NULL Err", __FUNCTION__);
        ret = s102_config_download_send(client, comm_guid, 0, "get object item IdentificationCameraAreaInfo NULL");
        return ret;
    }
    else
    {
        int identif_camera_area_size = cJSON_GetArraySize(identif_camera_area_json);
        for(int identif_i=0; identif_i<identif_camera_area_size; identif_i++)
        {
            cJSON *identif_area_json = cJSON_GetArrayItem(identif_camera_area_json, identif_i);
            if(identif_area_json == NULL)
            {
                ERR("%s:get object item IdentificationCameraAreaInfo[%d] NULL Err", __FUNCTION__, identif_i);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item IdentificationCameraAreaInfo NULL");
                return ret;
            }
            string camera_guid = get_json_string(identif_area_json, "CameraGuid");
            cJSON *lane_area_array_json = cJSON_GetObjectItem(identif_area_json, "LaneAreaInfos");
            if(lane_area_array_json == NULL)
            {
                ERR("%s:get object item LaneAreaInfos NULL Err", __FUNCTION__);
                ret = s102_config_download_send(client, comm_guid, 0, "get object item LaneAreaInfos NULL");
                return ret;
            }
            else
            {
                analyze_lane_area_infos(lane_area_array_json, camera_guid);
            }
        #if 0
            int lane_area_array_size = cJSON_GetArraySize(lane_area_array_json);
            for(int i=0; i<lane_area_array_size; i++)
            {
                cJSON *lane_area_json = cJSON_GetArrayItem(lane_area_array_json, i);
                if(lane_area_json == NULL)
                {
                    ERR("%s:get object item LaneAreaInfos NULL Err", __FUNCTION__);
                    ret = s102_config_download_send(client, comm_guid, 0, "get object item LaneAreaInfos NULL");
                    return ret;
                }
                string lane_guid = get_json_string(lane_area_json, "LaneGuid");
                //车道线
                cJSON *lane_line_array_json = cJSON_GetObjectItem(lane_area_json, "LaneLineInfos");
                if(lane_line_array_json != NULL)
                {
                    int lane_line_array_size = cJSON_GetArraySize(lane_line_array_json);
                    for(int j=0; j<lane_line_array_size; j++)
                    {
                        cJSON *lane_line_json = cJSON_GetArrayItem(lane_line_array_json, j);
                        if(lane_line_json == NULL)
                        {
                            ERR("%s:get object item LaneLineInfos[%d] NULL Err", __FUNCTION__, j);
                            break;
                        }
                        DB_Lane_Line_T lane_line;
                        lane_line.LaneGuid = lane_guid;
                        lane_line.LaneLineGuid = get_json_string(lane_line_json, "LaneLineGuid");
                        lane_line.Type = get_json_int(lane_line_json, "Type");
                        lane_line.Color = get_json_int(lane_line_json, "Color");
                        lane_line.CoordinateSet = get_json_string(lane_line_json, "CoordinateSet");
                        db_lane_line_insert(lane_line);
                    }
                }
                else
                {
                    ERR("%s:get object item LaneLineInfos NULL Err", __FUNCTION__);
                }
                //识别区
                cJSON *identif_area_array_json = cJSON_GetObjectItem(lane_area_json, "IdentificationAreas");
                if(identif_area_array_json != NULL)
                {
                    
                    int identif_area_array_size = cJSON_GetArraySize(identif_area_array_json);
                    for(int j=0; j<identif_area_array_size; j++)
                    {
                        cJSON *identif_area_json = cJSON_GetArrayItem(identif_area_array_json, j);
                        if(identif_area_json == NULL)
                        {
                            ERR("%s:get object item IdentificationAreas[%d] NULL Err", __FUNCTION__, j);
                            break;
                        }
                        DB_Lane_Identif_Area_T identif_area;
                        identif_area.LaneGuid = lane_guid;
                        identif_area.IdentifAreaGuid = get_json_string(identif_area_json, "Guid");
                        identif_area.Type = get_json_int(identif_area_json, "Type");
                        db_lane_identif_area_insert(identif_area);
                        cJSON *identif_area_p_array_json = cJSON_GetObjectItem(identif_area_json, "IdentificationAreaPoints");
                        if(identif_area_p_array_json != NULL)
                        {
                            
                            int identif_area_p_array_size = cJSON_GetArraySize(identif_area_p_array_json);
                            for(int m=0; m<identif_area_p_array_size; m++)
                            {
                                cJSON *identif_area_p_json = cJSON_GetArrayItem(identif_area_p_array_json, m);
                                if(identif_area_p_json == NULL)
                                {
                                    ERR("%s:get object item IdentificationAreaPoints[%d] NULL Err", __FUNCTION__, m);
                                    break;
                                }
                                DB_Lane_Identif_Area_Point_T identif_point;
                                identif_point.IdentifAreaGuid = identif_area.IdentifAreaGuid;
                                identif_point.SerialNumber = get_json_int(identif_area_p_json, "SerialNumber");
                                identif_point.XPixel = get_json_double(identif_area_p_json, "XPixel");
                                identif_point.YPixel = get_json_double(identif_area_p_json, "YPixel");
                                db_lane_identif_point_insert(identif_point);
                            }
                        }
                        else
                        {
                            ERR("%s:get object item IdentificationAreas[%d]IdentificationAreaPoints NULL Err", __FUNCTION__, j);
                            break;
                        }
                    }
                }
                else
                {
                    ERR("%s:get object item IdentificationAreas NULL Err", __FUNCTION__);
                }
                
            }
        #endif
        }
        
    }    

    //数据版本
    string DataVersion = get_json_string(data_json, "DataVersion");
    DBG("receive DataVersion:%s", DataVersion.c_str());
    if(DataVersion.length() > 1)
    {
        ret = update_conf_version(DataVersion.c_str());
        if(ret != 0){
            ERR("%s:update_conf_version() Err", __FUNCTION__);
        }
    }
    else
    {
        ERR("%s get PkaDataVersion err", __FUNCTION__);
        ret = -1;
    }

    if(ret < 0)
    {
        DBG("102 deal fail");
        ret = s102_config_download_send(client, comm_guid, 0, "fail");
    }
    else
    {
        DBG("102 deal success");
        ret = s102_config_download_send(client, comm_guid, 1, "ok");
    
    //    sleep(5);
    //    _exit(0);
        add_reboot_task();
    }

    return 0;
}

/*
 * 向eoc上传相机、雷达状态
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s103_dev_state_send(tcp_client_t* client)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_103);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_103);
    cJSON *radar_json = cJSON_CreateObject();
    cJSON_AddItemToObject(data, "RadarState", radar_json);
    UP_EOC_RadarState radar_state_data;
    get_up_radar_state(radar_state_data);
    cJSON_AddStringToObject(radar_json, "Guid", radar_state_data.Guid.c_str());
    if(radar_state_data.State == 1)
    {
        cJSON_AddNumberToObject(radar_json, "State", 0);    //eoc修改状态，0表示异常，1表示正常
    }
    else
    {
        cJSON_AddNumberToObject(radar_json, "State", 1);
    }
    cJSON_AddStringToObject(radar_json, "SoftVersion", radar_state_data.SoftVersion.c_str());
    
    cJSON *camera_array_json = cJSON_CreateArray();
    cJSON_AddItemToObject(data, "CameraStates", camera_array_json);
    vector <UP_EOC_CameraState> camera_state_date;
    get_up_camera_state(camera_state_date);
    for(unsigned int i=0; i<camera_state_date.size(); i++)
    {
        cJSON *camera_json;
        cJSON_AddItemToArray(camera_array_json,camera_json=cJSON_CreateObject());
        cJSON_AddStringToObject(camera_json, "Guid", camera_state_date[i].Guid.c_str());
        if(camera_state_date[i].State == 1)
        {
            cJSON_AddNumberToObject(camera_json, "State", 0);  //eoc修改状态，0表示异常，1表示正常
        }
        else
        {
            cJSON_AddNumberToObject(camera_json, "State", 1);
        }
        cJSON_AddStringToObject(camera_json, "Model", camera_state_date[i].Model.c_str());
        cJSON_AddStringToObject(camera_json, "SoftVersion", camera_state_date[i].SoftVersion.c_str());
        cJSON_AddStringToObject(camera_json, "DataVersion", camera_state_date[i].DataVersion.c_str());
        cJSON_AddNumberToObject(camera_json, "GetPicTotalCount", camera_state_date[i].GetPicTotalCount);
        cJSON_AddNumberToObject(camera_json, "GetPicSuccessCount", camera_state_date[i].GetPicSuccessCount);
    }
    
    json_str=cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";
    //DBG("s103_dev_state_send %s",send_str.c_str());
    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 处理eoc返回的状态上报信息
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int r103_dev_state_deal(cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    int state = get_json_int(data_json,"State");

    if(state == 1) {
        DBG("upload state success");
    } else {
        ERR("upload state fail, msg: %s", get_json_string(data_json,"Message").c_str());
    }

    return 0;
}
/*
 * 开启实时图片获取返回
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
int s104_start_real_time_pic_send(tcp_client_t* client, const char *comm_guid, Real_Time_Pic_Info &info_data)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;
   
    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    cJSON *element_array_json = cJSON_CreateArray();
    cJSON *radar_element_array_json = cJSON_CreateArray();
    cJSON *fusion_element_array_json = cJSON_CreateArray();
    if(root==NULL || data==NULL || element_array_json==NULL || radar_element_array_json==NULL 
            || fusion_element_array_json==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        if(element_array_json)cJSON_Delete(element_array_json);
        if(radar_element_array_json)cJSON_Delete(radar_element_array_json);
        if(fusion_element_array_json)cJSON_Delete(fusion_element_array_json);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_104);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code",EOC_COMM_SEND_104);
    //string pic_str = image2base64(info_data.PicPath.c_str());
    cJSON_AddStringToObject(data, "ImageStr", info_data.PicPath.c_str());
    cJSON_AddStringToObject(data, "TimeStamp", info_data.TimeStamp.c_str());
    cJSON_AddStringToObject(data, "CameraGuid", info_data.CameraGuid.c_str());
    cJSON_AddItemToObject(data, "ImgElements", element_array_json);
    for(unsigned int i=0; i<info_data.Elements.size(); i++)
    {
        cJSON *element_json;
        cJSON_AddItemToArray(element_array_json, element_json=cJSON_CreateObject());
        cJSON_AddStringToObject(element_json, "LaneGuid", info_data.Elements[i].LaneGuid.c_str());
        cJSON_AddStringToObject(element_json, "Id", info_data.Elements[i].Id.c_str());
        cJSON_AddStringToObject(element_json, "Longitude", info_data.Elements[i].Longitude.c_str());
        cJSON_AddStringToObject(element_json, "Latitude", info_data.Elements[i].Latitude.c_str());
        cJSON_AddNumberToObject(element_json, "LeftUpX", info_data.Elements[i].LeftUpX);
        cJSON_AddNumberToObject(element_json, "LeftUpY", info_data.Elements[i].LeftUpY);
        cJSON_AddNumberToObject(element_json, "RightDownX", info_data.Elements[i].RightDownX);
        cJSON_AddNumberToObject(element_json, "RightDownY", info_data.Elements[i].RightDownY);
        cJSON_AddNumberToObject(element_json, "CenterX", info_data.Elements[i].CenterX);
        cJSON_AddNumberToObject(element_json, "CenterY", info_data.Elements[i].CenterY);
        cJSON_AddNumberToObject(element_json, "LocationX", info_data.Elements[i].LocationX);
        cJSON_AddNumberToObject(element_json, "LocationY", info_data.Elements[i].LocationY);
    }
    cJSON_AddItemToObject(data, "RadarElements", radar_element_array_json);
    for(unsigned int i=0; i<info_data.RadarElements.size(); i++)
    {
        cJSON *element_json;
        cJSON_AddItemToArray(radar_element_array_json, element_json=cJSON_CreateObject());
        cJSON_AddStringToObject(element_json, "LaneGuid", info_data.RadarElements[i].LaneGuid.c_str());
        cJSON_AddStringToObject(element_json, "Id", info_data.RadarElements[i].Id.c_str());
        cJSON_AddStringToObject(element_json, "Longitude", info_data.RadarElements[i].Longitude.c_str());
        cJSON_AddStringToObject(element_json, "Latitude", info_data.RadarElements[i].Latitude.c_str());
        cJSON_AddNumberToObject(element_json, "LocationX", info_data.RadarElements[i].LocationX);
        cJSON_AddNumberToObject(element_json, "LocationY", info_data.RadarElements[i].LocationY);
    }
    cJSON_AddItemToObject(data, "FusionElements", fusion_element_array_json);
    for(unsigned int i=0; i<info_data.FusionElements.size(); i++)
    {
        cJSON *element_json;
        cJSON_AddItemToArray(fusion_element_array_json, element_json=cJSON_CreateObject());
        cJSON_AddStringToObject(element_json, "LaneGuid", info_data.FusionElements[i].LaneGuid.c_str());
        cJSON_AddStringToObject(element_json, "Id", info_data.FusionElements[i].Id.c_str());
        cJSON_AddStringToObject(element_json, "Longitude", info_data.FusionElements[i].Longitude.c_str());
        cJSON_AddStringToObject(element_json, "Latitude", info_data.FusionElements[i].Latitude.c_str());
        cJSON_AddNumberToObject(element_json, "LeftUpX", info_data.FusionElements[i].LeftUpX);
        cJSON_AddNumberToObject(element_json, "LeftUpY", info_data.FusionElements[i].LeftUpY);
        cJSON_AddNumberToObject(element_json, "RightDownX", info_data.FusionElements[i].RightDownX);
        cJSON_AddNumberToObject(element_json, "RightDownY", info_data.FusionElements[i].RightDownY);
        cJSON_AddNumberToObject(element_json, "CenterX", info_data.FusionElements[i].CenterX);
        cJSON_AddNumberToObject(element_json, "CenterY", info_data.FusionElements[i].CenterY);
        cJSON_AddNumberToObject(element_json, "LocationX", info_data.FusionElements[i].LocationX);
        cJSON_AddNumberToObject(element_json, "LocationY", info_data.FusionElements[i].LocationY);
    }

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 开始实时图片获取接口接收
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int r104_start_real_time_pic_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    string camera_guid = get_json_string(data_json, "CameraGuid");
    std::vector<string> lanes;

    cJSON *lane_array_json = cJSON_GetObjectItem(data_json, "LaneGuids");
    int lane_size = cJSON_GetArraySize(lane_array_json);
    for(int i=0; i<lane_size; i++)
    {
        string lane_guid = cJSON_GetArrayItem(lane_array_json, i)->valuestring;
        lanes.push_back(lane_guid);
    }
    Real_Time_Camera_Info camera_info;
    camera_info.CameraGuid = camera_guid;
    camera_info.CameraIp = get_camera_ip(camera_guid.c_str());
    camera_info.CommGuid = comm_guid;
    camera_info.TcpInfo = client;
    camera_info.Enable = 1;
    add_upload_pic_camera(camera_info);

    return 0;
}
/*
 * 停止实时图片获取返回
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s105_stop_real_time_pic_send(tcp_client_t* client, const char *comm_guid, int state)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;
   
    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_105);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code",EOC_COMM_SEND_105);
    cJSON_AddNumberToObject(data, "State", state);
    cJSON_AddStringToObject(data, "Message", "");

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 停止实时图片获取接口接收
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int r105_stop_real_time_pic_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    string camera_guid = get_json_string(data_json, "CameraGuid");
    std::vector<string> lanes;

    cJSON *lane_array_json = cJSON_GetObjectItem(data_json, "LaneGuids");
    int lane_size = cJSON_GetArraySize(lane_array_json);
    for(int i=0; i<lane_size; i++)
    {
        string lane_guid = cJSON_GetArrayItem(lane_array_json, i)->valuestring;
        lanes.push_back(lane_guid);
    }

    string camera_ip = get_camera_ip(camera_guid.c_str());
    update_upload_camera_state(camera_ip.c_str(), 0);

    ret = s105_stop_real_time_pic_send(client, comm_guid, 1);

    return ret;
}

/*
 * 向eoc发送获取配置
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s107_get_config_send(tcp_client_t* client)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;
    string equip_num;
    ret = db_factory_get_uname(equip_num);
    if(ret != 0){
        ERR("%s:get_factory_uname() Err", __FUNCTION__);
        return -1;
    }
//    equip_num = "aab65d77-8242-4f31-b088-46eb038663d9";

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_107);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code",EOC_COMM_SEND_107);
    cJSON_AddStringToObject(data, "MainboardGuid", equip_num.c_str());

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
static int r107_get_config_deal(tcp_client_t* client, cJSON *json)
{
    int ret = 0;
    char guid[8] = {0};
    memset(guid, 0, 8);
    ret = r102_config_download_deal(client, (const char *)guid, json);

    return ret;
}
//取外网状态，total外网通信总次数，success通信成功次数
int get_net_status(int *total, int *success)
{
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
static int s108_internet_status_send(tcp_client_t* client)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;
    string equip_num;
    ret = db_factory_get_uname(equip_num);
    if(ret != 0){
        ERR("%s:get_factory_uname() Err", __FUNCTION__);
        return -1;
    }
//    equip_num = "d7b39caf-41a7-4a26-b9a1-5999a7870619";

    //打包发送信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", random_uuid().data());
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_108);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code",EOC_COMM_SEND_108);
    cJSON_AddStringToObject(data, "MainboardGuid", equip_num.c_str());
    int total_cnt = 0;
    int success_cnt = 0;
    ret = get_net_status(&total_cnt, &success_cnt);
    cJSON_AddNumberToObject(data, "Total", total_cnt);
    cJSON_AddNumberToObject(data, "Success", success_cnt);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
static int r108_internet_status_deal(cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
    int state = get_json_int(data_json,"State");

    if(state == 1) {
        DBG("外网状态上传成功");
    } else {
        ERR("外网状态上传失败, msg: %s", get_json_string(data_json,"Message").c_str());
    }

    return 0;
}
/*
 * 相机软重启
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int s109_camera_reboot_send(tcp_client_t* client, const char *comm_guid, int type, int state, const char* msg)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;
   
    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_109);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code",EOC_COMM_SEND_109);
    cJSON_AddNumberToObject(data, "ControlType", type);
    cJSON_AddNumberToObject(data, "State", state);  //1=成功0=失败
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
static int r109_camera_reboot_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
    cJSON *data_json = cJSON_GetObjectItem(json, "Data");

    int control = get_json_int(data_json, "ControlType");
    if(control == 1)
    {
        //相机重启
        
    }

    ret = s109_camera_reboot_send(client, comm_guid, control, 1, "");

    return ret;
}
//雷视机下载、升级
int s110_download_send(tcp_client_t* client, const char *comm_guid, 
                            int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_110);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_110);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 处理雷视机软件下载文件指令，将会把文件下载到指定目录下
 * */
int r110_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
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

    ret = s110_download_send(client, comm_guid, 
                            1, 1, 0, "Receive controller download msg succeed");
    if(ret != 0)return ret;
    //判断文件是否存在，已存在直接返回下载成功
//    char update_file_name[256] = {0};
//    snprintf(update_file_name, 256, "%s_%s", rcv_download_msg.download_file_md5.c_str(),
//                rcv_download_msg.download_file_name.c_str());
    if(access(UPDATEFILE, R_OK|R_OK) == 0)
    {
        DBG("%s:%s 文件已存在", __FUNCTION__, UPDATEFILE);
        if(compute_file_md5(UPDATEFILE,  rcv_download_msg.download_file_md5.c_str()) == 0)
        {
            DBG("下载文件MD5校验通过");
            return s110_download_send(client, comm_guid, 2, 1, 0, "下载完成 MD5 OK");
        }
    }
    //添加下载任务
    ret = add_eoc_download_event(rcv_download_msg, rcv_dev_msg);
    if(ret < 0)
    {
        DBG("添加下载任务失败");
        return s110_download_send(client, comm_guid, 2, 0, 0, "add download_event Err");
    }
    else if(1 == ret)
    {
        DBG("下载任务存在，返回错误");
        return s110_download_send(client, comm_guid, 2, 0, 0, "该下载正在进行");
    }
    
    return 0;
}
int s111_upgrade_send(tcp_client_t* client, const char *comm_guid, 
                            int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_SEND_111);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_SEND_111);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
static int extract_file(const char *file_path)
{
    int ret = 0;
    char cmd[256] = {0};

    sprintf(cmd, "rm -rf %s", UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if(ret < 0){
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "mkdir -p %s", UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if(ret != 0){
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "tar -zxf %s -C %s", file_path, UPDATEUNZIPFILE);
    DBG("%s:Extract file start,cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if(ret < 0){
        ERR("%s:exec_cmd(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    if (remove(file_path) != 0)
    {
        ERR("remove %s error",file_path);
        return -1;
    }

    DBG("%s:Extract file succeed", __FUNCTION__);
    return 0;
}
static int start_upgrade()
{
    int ret = 0;
    DBG("%s:Enter.", __FUNCTION__);
    char cmd[256] = {0};

    sprintf(cmd, "chmod +x %s/%s", UPDATEUNZIPFILE, INSTALLSH);
    DBG("%s:cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if(ret < 0){
        ERR("%s:system(%s) Err", __FUNCTION__, cmd);
        return -1;
    }

    sprintf(cmd, "sh %s/%s %s", UPDATEUNZIPFILE, INSTALLSH, HOME_PATH);
    DBG("%s:cmd=%s", __FUNCTION__, cmd);
    ret = MySystemCmd(cmd);
    if(ret < 0){
        ERR("%s:system(%s) Err", __FUNCTION__, cmd);
        return -1;
    }
    DBG("%s:succeed", __FUNCTION__);
    return 0;
}
//相机升级
int r111_upgrade_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;
    EocUpgradeDev rcv_data;

    cJSON *data = cJSON_GetObjectItem(json, "Data");
    string file_md5 = get_json_string(data, "FileMD5");
    rcv_data.sw_version = get_json_string(data, "FileVersion");
    rcv_data.hw_version = get_json_string(data, "HardwareVersion");
    rcv_data.upgrade_mode = get_json_int(data, "UpgradeMode");
    string curr_sw_version = get_json_string(data, "CurrentSoftVersion");

    s111_upgrade_send(client, comm_guid, 1, 1, 0, "接收到升级命令");
    
    //1 判断文件是否存在
    if(access(UPDATEFILE, R_OK|R_OK) != 0){
        ERR("%s:%s 文件不存在", __FUNCTION__, UPDATEFILE);
        return s111_upgrade_send(client, comm_guid, 2, 0, 0, "文件不存在");
    }
    //2 MD5校验
    if(compute_file_md5(UPDATEFILE,  file_md5.c_str()) != 0){
        ERR("%s:%s MD5校验失败", __FUNCTION__, UPDATEFILE);
        return s111_upgrade_send(client, comm_guid, 2, 0, 0, "MD5校验失败");
    }
    //3 版本校验... ...
    
    
    ret = s111_upgrade_send(client, comm_guid, 1, 1, 25, "校验完成开始升级");
    //4 执行升级文件
    if(extract_file(UPDATEFILE) != 0){
        ERR("%s:%s 解压缩失败", __FUNCTION__, UPDATEFILE);
        return s111_upgrade_send(client, comm_guid, 2, 0, 0, "解压缩失败");
    }
    start_upgrade();
    ret = s111_upgrade_send(client, comm_guid, 2, 1, 0, "升级完成");
    
    return ret;
}

/*
 * 向eoc返回索要图片
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int sc004_get_img_send(tcp_client_t* client, const char *comm_guid, const char *camera_guid, 
                            const char *img_path, int state, int img_type)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return 0;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_CAMERA_S004);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_CAMERA_S004);
    cJSON_AddStringToObject(data, "ParkingAreaGuid", "");
    cJSON_AddStringToObject(data, "CameraGuid", camera_guid);
    cJSON_AddStringToObject(data, "ImagePath", img_path);    
    cJSON_AddNumberToObject(data, "State", state);
    cJSON_AddNumberToObject(data, "ImageType", img_type);
    cJSON_AddStringToObject(data, "Message", "");

    json_str=cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 处理eoc获取图片接口
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
//static int rc004_get_img_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
//{
//    int ret = 0;
//    cJSON *data_json = cJSON_GetObjectItem(json, "Data");
//    if(data_json == NULL)
//    {
//        ERR("%s:get object item Data NULL Err", __FUNCTION__);
//        sc004_get_img_send(client, comm_guid, "", "", 0, 0);
//        return 0;
//    }
//
//    char camera_ip[32] = {0};
//    sprintf(camera_ip, "%s", get_json_string(data_json,"CameraIP").c_str());
//    string camera_guid = get_json_string(data_json, "CameraGuid");
//    int compress_ratio = get_json_int(data_json, "CompressionRatios");
//    int img_type = get_json_int(data_json, "ImageType");
//    string img_url;
//    int state = upload_img_get_url(camera_ip, compress_ratio, img_type, img_url);
//
//    sc004_get_img_send(client, comm_guid, camera_guid.c_str(), img_url.c_str(), state, img_type);
//
//    return 0;
//}

/*
 * 向科研云发送相机配置返回
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int sc002_camera_detail_download_send(tcp_client_t* client, const char *comm_guid, int state, const char *msg)
{
    int ret = 0;
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* json_str = NULL;
    string send_str;

    //打包返回信息
    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() ERR", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return 0;
    }

    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_CAMERA_S002);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_CAMERA_S002);  
    cJSON_AddNumberToObject(data, "State", state);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);

    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}

/*
 * 相机配置下发处理
 *
 * 返回值：
 * 0：成功
 * -1：失败
 * */
static int rc002_camera_detail_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    int ret = 0;
//    char respons_msg[32] = {0};
    cJSON *data = cJSON_GetObjectItem(json, "Data");
    if(data == NULL)
    {
        ERR("%s:in json is NULL", __FUNCTION__);
        sc002_camera_detail_download_send(client, comm_guid, 0, "json data item null");
        return 0;
    }
    DB_Camera_Detail_Conf_T camera_data;
    
    camera_data.Guid = get_json_string(data, "Guid");
    camera_data.DataVersion = get_json_string(data, "DataVersion");

    cJSON *image_adjustment_json = cJSON_GetObjectItem(data, "ImageAdjustmentParam");
    if(image_adjustment_json == NULL) {
        ERR("%s:cJSON_GetObjectItem 'ImageAdjustmentParam' is NULL", __FUNCTION__);
        camera_data.Brightness = 0;
        camera_data.Contrast = 0;
        camera_data.Saturation = 0;
        camera_data.Acutance = 0;
        camera_data.MaximumGain = 0;
        camera_data.ExposureTime = 0;
    } else {
        camera_data.Brightness = get_json_int(image_adjustment_json, "Brightness");
        camera_data.Contrast = get_json_int(image_adjustment_json, "Contrast");
        camera_data.Saturation = get_json_int(image_adjustment_json, "Saturation");
        camera_data.Acutance = get_json_int(image_adjustment_json, "Acutance");
        camera_data.MaximumGain = get_json_int(image_adjustment_json, "MaximumGain");
        camera_data.ExposureTime = get_json_int(image_adjustment_json, "ExposureTime");
    }

    cJSON *image_enhance_json = cJSON_GetObjectItem(data, "ImageEnhanceParam");
    if(image_enhance_json == NULL) {
        ERR("%s:cJSON_GetObjectItem 'ImageEnhanceParam' is NULL", __FUNCTION__);
        camera_data.DigitalNoiseReduction = 0;
        camera_data.DigitalNoiseReductionLevel = 0;
    } else {
        camera_data.DigitalNoiseReduction = get_json_int(image_enhance_json, "DigitalNoiseReduction");
        camera_data.DigitalNoiseReductionLevel = get_json_int(image_enhance_json, "DigitalNoiseReductionLevel");
    }

    cJSON *back_light_json = cJSON_GetObjectItem(data, "BackLightAdjustmentParam");
    if(back_light_json == NULL) {
        ERR("%s:cJSON_GetObjectItem 'BackLightAdjustmentParam' is NULL", __FUNCTION__);
        camera_data.WideDynamic = 0;
        camera_data.WideDynamicLevel = 0;
    } else {
        camera_data.WideDynamic = get_json_int(back_light_json, "WideDynamic");
        camera_data.WideDynamicLevel = get_json_int(back_light_json, "WideDynamicLevel");
    }

    cJSON *video_codec_json = cJSON_GetObjectItem(data, "VideoCodingParam");
    if(video_codec_json == NULL) {
        ERR("%s:cJSON_GetObjectItem 'VideoCodingParam' is NULL", __FUNCTION__);
        camera_data.BitStreamType = 0;
        camera_data.Resolution = 8;
        camera_data.Fps = 0;
        camera_data.CodecType = 0;
        camera_data.ImageQuality = 0;
        camera_data.BitRateUpper = 0;
    } else {
        camera_data.BitStreamType = get_json_int(video_codec_json, "BitStreamType");
        camera_data.Resolution = get_json_int(video_codec_json, "Resolution");
        camera_data.Fps = get_json_int(video_codec_json, "Fps");
        camera_data.CodecType = get_json_int(video_codec_json, "CodecType");
        camera_data.ImageQuality = get_json_int(video_codec_json, "ImageQuality");
        camera_data.BitRateUpper = get_json_int(video_codec_json, "BitRateUpper");
    }

    cJSON *video_osd_json = cJSON_GetObjectItem(data, "VideoOsdParam");
    if(video_osd_json == NULL) {
        ERR("%s:cJSON_GetObjectItem 'VideoOsdParam' is NULL", __FUNCTION__);
        camera_data.IsShowName = 0;
        camera_data.IsShowDate = 8;
        camera_data.IsShowWeek = 0;
        camera_data.TimeFormat = 0;
        camera_data.DateFormat = "";
        camera_data.ChannelName = "";
        camera_data.FontSize = 1;
        camera_data.FontColor = "";
        camera_data.Transparency = 0;
        camera_data.Flicker = 0;
    } else {
        camera_data.IsShowName = get_json_int(video_osd_json, "IsShowName");
        camera_data.IsShowDate = get_json_int(video_osd_json, "IsShowDate");
        camera_data.IsShowWeek = get_json_int(video_osd_json, "IsShowWeek");
        camera_data.TimeFormat = get_json_int(video_osd_json, "TimeFormat");
        camera_data.DateFormat = get_json_string(video_osd_json, "DateFormat");
        camera_data.ChannelName = get_json_string(video_osd_json, "ChannelName");
        camera_data.FontSize = get_json_int(video_osd_json, "FontSize");
        camera_data.FontColor = get_json_string(video_osd_json, "FontColor");
        camera_data.Transparency = get_json_int(video_osd_json, "Transparency");
        camera_data.Flicker = get_json_int(video_osd_json, "Flicker");
    }
    //配置相应相机
    ret = db_camera_detail_set_store(camera_data);
    if(ret != 0){
        ERR("%s:db_camera_detail_set_store() Err", __FUNCTION__);
        sc002_camera_detail_download_send(client, comm_guid, 0, "store data err");
        return 0;
    }

    //更新上传状态相机详细配置版本号
    update_camera_data_version(camera_data.Guid.c_str(), camera_data.DataVersion.c_str());

    sc002_camera_detail_download_send(client, comm_guid, 1, "success");

    return 0;
}
//相机下载、升级
int sc006_download_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_CAMERA_S006);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_CAMERA_S006);
    cJSON_AddStringToObject(data, "CameraGuid", guid);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
/*
 * 处理相机软件升级的下载文件指令，将会把文件下载到指定目录下
 * */
int rc006_download_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
{
    DBG("%s:Enter.", __FUNCTION__);
    int ret = 0;

    cJSON *data = cJSON_GetObjectItem(json, "Data");

    EocDownloadsMsg rcv_download_msg;
    EocUpgradeDev rcv_dev_msg;
    rcv_download_msg.download_file_name = get_json_string(data, "Filename");
    rcv_download_msg.download_file_size = atoi(get_json_string(data, "FileSize").c_str());
    rcv_download_msg.download_url = get_json_string(data, "DownloadPath");
    rcv_download_msg.download_file_md5 = get_json_string(data, "FileMD5");    
    rcv_dev_msg.dev_guid = get_json_string(data, "CameraGuid");
    rcv_dev_msg.dev_ip = get_json_string(data, "CameraIP");
    rcv_dev_msg.dev_type = EOC_UPGRADE_CAMARA;
    rcv_dev_msg.comm_guid = comm_guid;

    ret = sc006_download_send(client, comm_guid, rcv_dev_msg.dev_guid.c_str(), 
                            1, 1, 0, "Receive controller download msg succeed");
    if(ret != 0)return ret;
    //判断文件是否存在，已存在直接返回下载成功
    char update_file_name[256] = {0};
    snprintf(update_file_name, 256, "%s_%s", rcv_download_msg.download_file_md5.c_str(),
                rcv_download_msg.download_file_name.c_str());
    if(access(update_file_name, R_OK|R_OK) == 0)
    {
        DBG("%s:%s 文件已存在", __FUNCTION__, update_file_name);
        if(compute_file_md5(update_file_name,  rcv_download_msg.download_file_md5.c_str()) == 0)
        {
            DBG("下载文件MD5校验通过");
            return sc006_download_send(client, comm_guid, rcv_dev_msg.dev_guid.c_str(), 
                                                    2, 1, 0, "下载完成 MD5 OK");
        }
    }
    //添加下载任务
    ret = add_eoc_download_event(rcv_download_msg, rcv_dev_msg);
    if(ret < 0)
    {
        DBG("添加下载任务失败");
        return sc006_download_send(client, comm_guid, rcv_dev_msg.dev_guid.c_str(), 
                                                    2, 0, 0, "add download_event Err");
    }
    else if(1 == ret)
    {
        DBG("下载任务存在，返回错误");
        return sc006_download_send(client, comm_guid, rcv_dev_msg.dev_guid.c_str(), 2, 0, 0, "该下载正在进行");
    }
    
    return 0;
}
int sc007_upgrade_send(tcp_client_t* client, const char *comm_guid, 
                            const char *guid, int result, int stats, int progress, const char *msg)
{
    int ret = -1;
    cJSON *root = NULL,*data = NULL;
    char *json_str = NULL;
    string send_str;

    root=cJSON_CreateObject();
    data=cJSON_CreateObject();
    if(root==NULL || data==NULL){
        ERR("%s:cJSON_CreateObject() Err", __FUNCTION__);
        if(root)cJSON_Delete(root);
        if(data)cJSON_Delete(data);
        return -1;
    }
    //打包发送信息
    cJSON_AddStringToObject(root, "Guid", comm_guid);
    cJSON_AddStringToObject(root, "Code", EOC_COMM_CAMERA_S007);
    cJSON_AddStringToObject(root, "Version", EOC_COMMUNICATION_VERSION);
    cJSON_AddItemToObject(root, "Data", data);
    //填充data
    cJSON_AddStringToObject(data, "Code", EOC_COMM_CAMERA_S007);
    cJSON_AddStringToObject(data, "CameraGuid", guid);
    cJSON_AddNumberToObject(data, "ResultType", result);
    cJSON_AddNumberToObject(data, "State", stats);
    cJSON_AddNumberToObject(data, "Progress", progress);
    cJSON_AddStringToObject(data, "Message", msg);

    json_str=cJSON_PrintUnformatted(root);
    send_str = json_str;
    send_str += "*";

    ret = tcp_client_write(client, (char*)send_str.c_str(), send_str.length());
    if( ret < 0) {
        ERR("%s cloud send failed!", __FUNCTION__);
        ret = -1;
    }else{
        ret = 0;
    }

    cJSON_Delete(root);
    free(json_str);
    return ret;
}
//相机升级
//int rc007_upgrade_deal(tcp_client_t* client, const char *comm_guid, cJSON *json)
//{
//    DBG("%s:Enter.", __FUNCTION__);
//    int ret = 0;
//    char update_file_name[128] = {0};
//    EocUpgradeDev rcv_data;
//
//    cJSON *data = cJSON_GetObjectItem(json, "Data");
//    string file_md5 = get_json_string(data, "FileMD5");
//    snprintf(update_file_name, 128, "%s_%s", file_md5.c_str(), get_json_string(data, "Filename").c_str());
//    rcv_data.dev_guid = get_json_string(data, "CameraGuid");
//    rcv_data.dev_ip = get_json_string(data, "CameraIP");
//    rcv_data.dev_model = get_json_string(data, "CameraModel");
//    rcv_data.sw_version = get_json_string(data, "FileVersion");
//    rcv_data.hw_version = get_json_string(data, "HardwareVersion");
//    rcv_data.upgrade_mode = get_json_int(data, "UpgradeMode");
//    string curr_sw_version = get_json_string(data, "CurrentSoftVersion");
//
//    sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 1, 1, 0, "接收到升级命令");
//
//    //1 判断文件是否存在
//    if(access(update_file_name, R_OK|R_OK) != 0){
//        ERR("%s:%s 文件不存在", __FUNCTION__, update_file_name);
//        return sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "文件不存在");
//    }
//    //2 MD5校验
//    if(compute_file_md5(update_file_name,  file_md5.c_str()) != 0){
//        ERR("%s:%s MD5校验失败", __FUNCTION__, update_file_name);
//        return sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "MD5校验失败");
//    }
//    //3 版本校验... ...
//
//    //添加升级任务
//    rcv_data.dev_type = EOC_UPGRADE_CAMARA;
//    rcv_data.comm_guid = comm_guid;
//    rcv_data.up_status = EOC_UPGRADE_IDLE;
//    rcv_data.up_progress = 10;
//    ret = add_eoc_upgrade_event(rcv_data);
//    if(0 > ret)
//    {
//        ret = sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "添加升级任务失败");
//    }
//    else if(1 == ret)
//    {
//        ret = sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 2, 0, 0, "升级任务已存在");
//    }
//    else if(0 == ret)
//    {
//        ///启动相机升级
//        if('X' == rcv_data.dev_model.c_str()[0])
//        {
//            xm_camera_auto_upgrade(rcv_data.dev_ip.c_str(), rcv_data.dev_model.c_str(), curr_sw_version.c_str(),
//                                    rcv_data.sw_version.c_str(), update_file_name);
//        }
//        ret = sc007_upgrade_send(client, comm_guid, rcv_data.dev_guid.c_str(), 1, 1, 10, "校验完成开始升级");
//    }
//
//    return ret;
//}
//向EOC返回相机升级状态 返回值：<0发送错误，0升级中发送状态完成，1升级结束
//int send_camera_upgrade_state(tcp_client_t* client, EocUpgradeDev *upgrade_msg)
//{
//    int ret = 0;
//
//    AUTO_UPGRADE_STATE state;
//    bool upgrade_ret = 0;
//    int step_total = 0;
//    int current_step = 0;
//    int progress = 0;
//    std::string message;
//
//    ////////?取相机升级进度
//    if('X' == upgrade_msg->dev_model.c_str()[0])
//    {
//        upgrade_ret = xm_camera_get_auto_upgrade_state(upgrade_msg->dev_ip.c_str(), &state,
//                            &step_total, &current_step, &progress, message);
//    }
//    else
//    {
//        //暂只支持雄迈相机升级
//        ret = sc007_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(),
//                            2, 0, progress, "暂不支持该型号相机升级");
//        return 1;
//    }
//
//    if(UPGRADE_NOT_DOING == state)
//    {
//    }
//    else if(UPGRADE_DOING == state)
//    {
//        ret = sc007_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(),
//                            1, 1, progress, "upgrading");
//        return 0;
//    }
//    else if(UPGRADE_SUCCEED == state)
//    {
//        ret = sc007_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(),
//                         2, 1, 0, "升级成功");
//        return 1;
//    }
//    else if(UPGRADE_FAILED == state)
//    {
//        ret = sc007_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(),
//                            2, 0, progress, "升级失败");
//        return 1;
//    }
//
//    if(time(NULL) >= upgrade_msg->start_upgrade_time + 5*60)
//    {
//        ret = sc007_upgrade_send(client, upgrade_msg->comm_guid.c_str(), upgrade_msg->dev_guid.c_str(),
//                            2, 0, progress, "升级超时");
//        return 1;
//    }
//
//    return 0;
//}


//eoc通信数据初始化eoc_comm_data
int comm_data_init(void)
{
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
BLUE_TCP_STATE connected_callback(tcp_client_t* client)
{
    /*
     * 1、初始化EOC状态
     * */
    is_login = false;
    heart_flag = 0;
    eoc_rec_data.clear();
    comm_data_init();
    
    int ret = 0;
    ret = s101_login_send(client);
    if(ret < 0)
    {
        ERR("s101_login_send err, return %d", ret);
        return (BLUE_TCP_STATE)1;
    }
    else
    {
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
    return (BLUE_TCP_STATE)0;
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
BLUE_TCP_STATE datacoming_callback(tcp_client_t* client, char* data, size_t data_size)
{
    if(strlen(data) <= 0)
    {
        ERR("receive data null");
        return (BLUE_TCP_STATE)0;
    }
    if(data[strlen(data)-1] != '*')
    {
        eoc_rec_data += data;
        DBG("接收不全%s", data);
        return (BLUE_TCP_STATE)0;
    }
    eoc_rec_data += data;

    int ret = 0;
    vector<string> sv;
    //处理粘包
    ret = splitstr2vector(eoc_rec_data.c_str(), "*", sv);
    if(ret == 0){
//      DBG("buf=%s", buf);
        for(string &real_json_str:sv){
            cJSON *json;
            char *rec_out;
            char code[10],guid[37];
            char *tmpbuf;

            int buf_len = (int)strlen(real_json_str.c_str());
            int rec_out_len = 0;

//          DBG("real_json_str=%s", real_json_str.c_str());
            //数据解析
            json=cJSON_Parse(real_json_str.c_str());//字符串解析成json结构体
            if (json == NULL) {
                ERR("Error before: [%s]",cJSON_GetErrorPtr());
                ERR("不是json结构体:%s", real_json_str.c_str());
                continue;
            }
/*
            tmpbuf = (char *)malloc(1024);
            if (tmpbuf == NULL)
            {
                ERR("申请内存失败");
                cJSON_Delete(json);
                return (BLUE_TCP_STATE)1;
            }
*/
            rec_out=cJSON_PrintUnformatted(json);//json结构体转换成字符串
            rec_out_len = (int)strlen(rec_out);
        //    DBG("buf strlen %d, rec_out strlen %d", buf_len, rec_out_len);
/*            for(int i = 0;i <= (int)strlen(rec_out)/1000;i++)
            {
                memset(tmpbuf,0x0,1024);
                snprintf(tmpbuf,1000+1,"%s",rec_out + i*1000);
                DBG("receive from keyan cloud:%s",tmpbuf);
            }
            free(tmpbuf);
*/
            sprintf(code, "%s", get_json_string(json,"Code").c_str());//读取指令码

            if(strcmp(code, EOC_COMM_RECEIVE_100) == 0){
                ret = r100_heart_deal(json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_101) == 0){
                ret = r101_login_deal(json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_102) == 0){
                ret = r102_config_download_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_103) == 0){
                ret = r103_dev_state_deal(json);
            }/*else if(strcmp(code, EOC_COMM_CAMERA_R004) == 0){
                ret = rc004_get_img_deal(client, get_json_string(json,"Guid").c_str(), json);
            }*/else if(strcmp(code, EOC_COMM_CAMERA_R002) == 0){
                ret = rc002_camera_detail_download_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_CAMERA_R006) == 0){
                ret = rc006_download_deal(client, get_json_string(json,"Guid").c_str(), json);
            }/*else if(strcmp(code, EOC_COMM_CAMERA_R007) == 0){
                ret = rc007_upgrade_deal(client, get_json_string(json,"Guid").c_str(), json);
            }*/else if(strcmp(code, EOC_COMM_RECEIVE_104) == 0){
                ret = r104_start_real_time_pic_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_105) == 0){
                ret = r105_stop_real_time_pic_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_107) == 0){
                ret = r107_get_config_deal(client, json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_108) == 0){
                ret = r108_internet_status_deal(json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_109) == 0){
                ret = r109_camera_reboot_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_110) == 0){
                ret = r110_download_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_COMM_RECEIVE_111) == 0){
                ret = r111_upgrade_deal(client, get_json_string(json,"Guid").c_str(), json);
            }
            else if(strcmp(code, EOC_CONTROLLER_R001) == 0){
                ret = ckr001_status_deal(json);
            }else if(strcmp(code, EOC_CONTROLLER_R002) == 0){
                ret = ckr002_server_get_status_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R003) == 0){
                ret = ckr003_fan_control_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R004) == 0){
                ret = ckr004_heat_control_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R005) == 0){
                ret = ckr005_angle_control_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R006) == 0){
                ret = ckr006_led_control_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R007) == 0){
                ret = ckr007_relay_control_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R008) == 0){
                ret = ckr008_download_deal(client, get_json_string(json,"Guid").c_str(), json);
            }else if(strcmp(code, EOC_CONTROLLER_R009) == 0){
                ret = ckr009_upgrade_deal(client, get_json_string(json,"Guid").c_str(), json);
            }
            else{
                WARNING("%s:UNKNOWN code[%s]", __FUNCTION__, code);
            }
            //最后退出
            cJSON_Delete(json);
            free(rec_out);
        }
    }else{
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
    if(ret < 0)
    {
        return (BLUE_TCP_STATE)1;
    }
    else
    {
        return (BLUE_TCP_STATE)0;
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
BLUE_TCP_STATE interval_callback(tcp_client_t* client)
{
    int ret = 0;
    static time_t last_login_t = time(NULL);
    static time_t last_send_heart_t = time(NULL);
    static time_t send_103_time = time(NULL)-60;   //设备状态
    static time_t send_net_state_time = time(NULL)-190;   //外网状态
    static time_t send_k1_stat_time = time(NULL)-185;   //矩阵控制器状态
//    static time_t send_106_time = time(NULL)-30;   //RFID状态
//    static time_t send_107_time = time(NULL)-40;   //GPS状态
//    static time_t send_108_time = time(NULL)-50;   //停车区域状态
    static time_t get_config_time = 0;

    time_t now = time(NULL);

//    last_login_t = now - 3;
//    DBG("now[%ld] - last_login_t[%ld] = %ld", now, last_login_t, now - last_login_t);
    if(is_login == false && now - last_login_t > 10)
    {
        last_login_t = now;
        ret = s101_login_send(client);
        if(ret < 0)
        {
            ERR("s101_login_send err, return %d", ret);
            return (BLUE_TCP_STATE)1;
        }
        else
        {
            DBG("s101_login_send ok");
        }
        last_send_heart_t = now -20;
    }

    if(is_login == true)
    {
        if((now-get_config_time>180) && (db_conf_version.length()==0))
        {
            get_config_time = now;

            DBG("s107_get_config_send");
            ret = s107_get_config_send(client);
            if(ret < 0)
            {
                ERR("s107_get_config_send err, return %d", ret);
                return (BLUE_TCP_STATE)1;
            }
            else
            {
                DBG("s107_get_config_send ok");
            }
        }
        //心跳
        if(now - last_send_heart_t > 30)
        {
            last_send_heart_t = now;
            /*
             * 发送心跳状态
             * */
            ret = s100_heart_send(client);
            if(ret < 0)
            {
                ERR("s100_heart_send err, return %d", ret);
                return (BLUE_TCP_STATE)1;
            }
            else
            {
                DBG("s100_heart_send ok");
            }
            heart_flag ++;
            if(heart_flag > 10)
            {
                ERR("心跳连续10次未回复");
                return (BLUE_TCP_STATE)1;
            }
        }
        //设备状态上传
        if(now - send_103_time > 60)
        {
            send_103_time = now;
            ret = s103_dev_state_send(client);
            if(ret < 0)
            {
                ERR("s103_dev_state_send err, return %d", ret);
                return (BLUE_TCP_STATE)1;
            }
            else
            {
                DBG("s103_dev_state_send ok");
            }
        }
        //外网状态上传
        if(now - send_net_state_time > 300)
        {
            send_net_state_time = now;
            ret = s108_internet_status_send(client);
            if(ret < 0)
            {
                ERR("s108_internet_status_send err, return %d", ret);
                return (BLUE_TCP_STATE)1;
            }
            else
            {
                DBG("s108_internet_status_send ok");
            }
        }
        //矩阵控制器状态
        if(now - send_k1_stat_time > 300)
        {
            send_k1_stat_time = now;
            char comm_guid[8] = {0};
            memset(comm_guid, 0, 8);
            ret = cks001_status_send(client, comm_guid);
            if(ret < 0)
            {
                ERR("cks001_status_send err, return %d", ret);
                return (BLUE_TCP_STATE)1;
            }
            else
            {
                DBG("cks001_status_send ok");
            }
        }

        //升级、下载
        //处理下载消息
        ret = 0;
        for(auto it = eoc_comm_data.downloads_msg.begin(); it != eoc_comm_data.downloads_msg.end();)
        {
            if (it->download_status != DOWNLOAD_IDLE)
            {
                ret = send_download_msg(client, &(*it));
                it = eoc_comm_data.downloads_msg.erase(it); //删除元素，返回值指向已删除元素的下一个位置
                if(ret != 0){
                    ERR("send_download_msg() err, ret=%d", ret);
                    break;
                }
            }
            else
            {
                ++it; //指向下一个位置    
            } 
        }
        //处理升级消息
        ret = 0;
        for(auto it = eoc_comm_data.upgrade_msg.begin(); it != eoc_comm_data.upgrade_msg.end();)
        {
//            if(it->dev_type == EOC_UPGRADE_CAMARA)
//            {
//                //取相机的升级进度返回
//                ret = send_camera_upgrade_state(client, &(*it));
//                if(ret == 1)
//                {
//                    it = eoc_comm_data.upgrade_msg.erase(it); //升级完成，删除元素，返回值指向已删除元素的下一个位置
//                }
//                else if(ret == 0)
//                {
//                    ++it; //指向下一个位置
//                }
//                else if(ret < 0)
//                {
//                    ERR("send_camera_upgrade_state() err");
//                    break;
//                }
//            //    it = eoc_comm_data.upgrade_msg.erase(it); //删除元素
//            }
            /*else*/ if(it->dev_type == EOC_UPGRADE_CONTROLLER)
            {
                //取矩阵控制器升级进度返回
                ret = send_ck1_upgrade_state(client, &(*it));
                if(ret == 1)
                {
                    del_c500_upgrade_task(it->dev_ip.c_str());
                    it = eoc_comm_data.upgrade_msg.erase(it); //升级完成，删除元素，返回值指向已删除元素的下一个位置
                }
                else
                {
                    ++it; //指向下一个位置
                }
            }
            else
            {
                ERR("升级类型错误，dev_type=%d", it->dev_type);
                it = eoc_comm_data.upgrade_msg.erase(it); //删除元素
            }
        }
        //任务处理
        if(eoc_comm_data.task.size()){
            EOCCLOUD_TASK task = eoc_comm_data.task[0];
            eoc_comm_data.task.erase(eoc_comm_data.task.begin());
            if(task == SYS_REBOOT){
                if((eoc_comm_data.task.size()>0) || (eoc_comm_data.downloads_msg.size()>0))
                {
                    DBG("有其他任务正在进行，延后重启");
                    //重新添加重启任务
                    add_reboot_task();
                }
                else
                {
                    DBG("running task SYS_REBOOT");
                    sleep(5);
                    _exit(0);
                }
            }else if(task == AUTO_UPGRADE){
        #if 0
                DBG("running task AUTO_UPGRADE");
                ret = s115_auto_upgrade_send(client);
                if(ret < 0)
                {
                    ERR("s115_auto_upgrade_send err, return %d", ret);
                    return (BLUE_TCP_STATE)1;
                }
                else
                {
                    DBG("s115_auto_upgrade_send ok");
                }
        #endif
            }
            else
            {
                DBG("task = %d unsupport", (int)task);
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
    return (BLUE_TCP_STATE)0;
}

/*
 * TCP断开连接后回调函数
 *
 * 参数：无
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
BLUE_TCP_STATE disconnected_callback()
{
    DBG("disconnected_callback");
    return (BLUE_TCP_STATE)0;
}



int eoc_communication_start(const char *server_path, int server_port)
{
    DBG("EOC communication Running");    
    clear_up_camera_state();
    clear_up_radar_state();
    clear_upload_camera();
    start_k1_comm();
    tcp_client_t* client = tcp_client_create("yylsj-eoc communication", server_path, server_port, connected_callback,
            datacoming_callback, disconnected_callback, interval_callback, 1);
    /*雄迈相机设置默认参数*////?
//    xm_camera_param_default_set();
    ////雄迈相机升级添加
//    xm_camera_assist_init();
    //雄迈相机SDK会设置SIGCHLD信号的处理，此处恢复默认处理方式，否则waitpid返回-1导致MySestemCmd执行命令失败
    signal(SIGCHLD, SIG_DFL);
    
    return 0;
}



