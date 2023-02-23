//
// Created by lining on 2023/2/23.
//

#include "DNSServer.h"

/*dns服务，ip池*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <map>
#include "dns_struct.h"
#include "uri.h"
#include <netdb.h>
#include <vector>

using namespace std;

/*判断字符串是不是一个有效ip地址,0非ip，1，ip地址*/
int isIP(char *str)
{
    char temp[32];
    int a,b,c,d;

    if (strlen(str) > strlen("255.255.255.255"))
    {
        return 0;
    }

    if((sscanf(str,"%d.%d.%d.%d",&a,&b,&c,&d))!=4)
    {
        return 0;
    }
    sprintf(temp,"%d.%d.%d.%d",a,b,c,d);
    if(strcmp(temp,str) != 0)
    {
        return 0;
    }
    if(!((a <= 255 && a >= 0)&&(b <= 255 && b >= 0)&&(c <= 255 && c >= 0)))
    {
        return 0;
    }

    return 1;
}

/*
参数：
1）host_name，需要解析的域名
2）host_ip，解析出的IP
返回值：0成功，-1失败
*/
int dns_resolve_114(const char *dnsip,const char* host_name, char *host_ip)
{
#define BUF_SIZE 1024
#define SRV_PORT 53

    if(NULL == host_name || NULL == host_ip)
    {
        ERR("dns_resolve_114 host_name=%s host_ip=%s",host_name,host_ip);
        return -1;
    }

    typedef unsigned short U16;
    //const char srv_ip[] = "114.114.114.114";
    typedef struct _DNS_HDR
    {
        U16 id;
        U16 tag;
        U16 numq;
        U16 numa;
        U16 numa1;
        U16 numa2;
    }DNS_HDR;
    typedef struct _DNS_QER
    {
        U16 type;
        U16 classes;
    }DNS_QER;

    int      clifd,len = 0,i;
    struct   sockaddr_in servaddr;
    char     buf[BUF_SIZE];
    char     *ptr;
    char     *tmp_ptr;
    DNS_HDR  *dnshdr = (DNS_HDR *)buf;
    DNS_QER  *dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR));

    timeval tv = {5, 0};

    if ((clifd  =  socket(AF_INET, SOCK_DGRAM, 0 ))  < 0)
    {
        ERR("dns_resolve_114 create socket error!" );
        return -1;
    }
    setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_aton(dnsip, &servaddr.sin_addr);
    servaddr.sin_port = htons(SRV_PORT);
    memset(buf, 0, BUF_SIZE);
    dnshdr->id = (U16)1;
    dnshdr->tag = htons(0x0100);
    dnshdr->numq = htons(1);
    dnshdr->numa = 0;

    strcpy(buf + sizeof(DNS_HDR) + 1, host_name);
    ptr = buf + sizeof(DNS_HDR) + 1;
    i = 0;
    while (ptr < (buf + sizeof(DNS_HDR) + 1 + strlen(host_name)))
    {
        if ( *ptr == '.'){
            *(ptr - i - 1) = i;
            i = 0;
        }
        else{
            i++;
        }
        ptr++;
    }
    *(ptr - i - 1) = i;

    dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR) + 2 + strlen(host_name));
    dnsqer->classes = htons(1);
    dnsqer->type = htons(1);
    len = sendto(clifd, buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + strlen(host_name) + 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    i = sizeof(struct sockaddr_in);
    len = recvfrom(clifd, buf, BUF_SIZE, 0, (struct sockaddr *)&servaddr, (socklen_t *)&i);
    close(clifd);
    if (len < 0)
    {
        ERR("dns_resolve_114 recv error");
        return -1;
    }
    if (dnshdr->numa == 0)
    {
        ERR("dns_resolve_114 ack error");
        return -1;
    }
    ptr = buf + len -4;
    tmp_ptr = ptr;

    std::vector<std::string> dns_ip_str_all;
    dns_parse(buf,len,dns_ip_str_all);

    if (dns_ip_str_all.size() == 0)
    {
        ERR("dns_resolve_114 dns_parse error");
        return -1;
    }

    for(int i = 0;i < (int)dns_ip_str_all.size();i++)
    {
        DBG("host:%s;解析dns:%s",host_name,(char *)dns_ip_str_all[i].c_str());
    }

    //sprintf(host_ip,"%u.%u.%u.%u",(unsigned char)*ptr, (unsigned char)*(ptr + 1), (unsigned char)*(ptr + 2), (unsigned char)*(ptr + 3));
    sprintf(host_ip,"%s",(char *)dns_ip_str_all[0].c_str());
    return 0;
}


#if 0
/*
参数：
1）host_name，需要解析的域名
2）host_ip，解析出的IP
返回值：0成功，-1失败
*/
int dns_resolve_114(char *dnsip,const char* host_name, char *host_ip)
{
#define BUF_SIZE 1024
#define SRV_PORT 53

    if(NULL == host_name || NULL == host_ip)
    {
        ERR("dns_resolve_114 host_name=%s host_ip=%s",host_name,host_ip);
        return -1;
    }

    typedef unsigned short U16;
    //const char srv_ip[] = "114.114.114.114";
    typedef struct _DNS_HDR
    {
      U16 id;
      U16 tag;
      U16 numq;
      U16 numa;
      U16 numa1;
      U16 numa2;
    }DNS_HDR;
    typedef struct _DNS_QER
    {
       U16 type;
       U16 classes;
    }DNS_QER;

    int      clifd,len = 0,i;
    struct   sockaddr_in servaddr;
    char     buf[BUF_SIZE];
    char     *ptr;
    DNS_HDR  *dnshdr = (DNS_HDR *)buf;
    DNS_QER  *dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR));
    timeval tv = {10, 0};

    if ((clifd  =  socket(AF_INET, SOCK_DGRAM, 0 ))  < 0)
    {
         ERR("dns_resolve_114 create socket error!" );
         return -1;
    }
    setsockopt(clifd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(timeval));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_aton(dnsip, &servaddr.sin_addr);
    servaddr.sin_port = htons(SRV_PORT);
    memset(buf, 0, BUF_SIZE);
    dnshdr->id = (U16)1;
    dnshdr->tag = htons(0x0100);
    dnshdr->numq = htons(1);
    dnshdr->numa = 0;

    strcpy(buf + sizeof(DNS_HDR) + 1, host_name);
    ptr = buf + sizeof(DNS_HDR) + 1;
    i = 0;
    while (ptr < (buf + sizeof(DNS_HDR) + 1 + strlen(host_name)))
    {
        if ( *ptr == '.'){
            *(ptr - i - 1) = i;
            i = 0;
        }
        else{
            i++;
        }
        ptr++;
    }
    *(ptr - i - 1) = i;

    dnsqer = (DNS_QER *)(buf + sizeof(DNS_HDR) + 2 + strlen(host_name));
    dnsqer->classes = htons(1);
    dnsqer->type = htons(1);
    len = sendto(clifd, buf, sizeof(DNS_HDR) + sizeof(DNS_QER) + strlen(host_name) + 2, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    i = sizeof(struct sockaddr_in);
    len = recvfrom(clifd, buf, BUF_SIZE, 0, (struct sockaddr *)&servaddr, (socklen_t *)&i);
    close(clifd);
    if (len < 0)
    {
          ERR("dns_resolve_114 recv error");
          return -1;
    }
    if (dnshdr->numa == 0)
    {
          ERR("dns_resolve_114 ack error");
          return -1;
    }
    ptr = buf + len -4;
    DBG("dns_resolve_114 解析dns[%s]成功%s ==> %u.%u.%u.%u\n",dnsip,host_name, (unsigned char)*ptr, (unsigned char)*(ptr + 1), (unsigned char)*(ptr + 2), (unsigned char)*(ptr + 3));
    sprintf(host_ip,"%u.%u.%u.%u",(unsigned char)*ptr, (unsigned char)*(ptr + 1), (unsigned char)*(ptr + 2), (unsigned char)*(ptr + 3));
    return 0;
}
#endif

/*解析url*/
int uri_parse(char *sorcpath, char *urihost, char *uripath)
{
    struct ast_uri* uri_s = ast_uri_parse(sorcpath);

    if(uri_s->host != NULL) {
        sprintf(urihost,"%s",uri_s->host);
    }

    if(uri_s->path != NULL) {
        sprintf(uripath,"/%s",uri_s->path);
    }

    free(uri_s);

    return 0;
}

/*解析dns:

hoststr:域名;
ipaddr:ip地址;
ret = 0 成功
*/
int url_get(std::string hoststr,std::string &ipaddr)
{
    struct  hostent hostinfo,*phost;
    char tmpipaddr[64];
    char *buf;
    int ret;
    char dnsserver[8][32] = {
            "119.29.29.29", 		//public DNS+
            "182.254.116.116",		//public DNS+
            "114.114.114.114",
            "223.5.5.5",			//ali
            "223.6.6.6",			//ali
            "180.76.76.76", 		//baidu
            "8.8.8.8",				//google
            "8.8.4.4"				//google
    };
    buf = (char *)malloc(1024);
    if (buf == NULL)
    {
        ERR("malloc错误");
        return -1;
    }
    memset(buf,0x0,1024);
    memset(tmpipaddr,0x0,sizeof(tmpipaddr));

    /*取得主机IP地址*/
    if(gethostbyname_r((char *)hoststr.c_str(),&hostinfo,buf,
                       1024,&phost,&ret))
    {
        free(buf);
        ERR("ERROR:gethostbyname_r(%s) ret:%d",
            hoststr.c_str(),ret);
        int i;
        int size = sizeof(dnsserver)/64;

        for(i = 0;i < size;i++)
        {
            ret = dns_resolve_114(dnsserver[i],(char *)hoststr.c_str(),
                                  tmpipaddr);
            if (ret < 0)
            {
                ERR("dns_resolve_114 通过%s获取ip地址失败",dnsserver[i]);
                continue;
            }
            ipaddr = tmpipaddr;
            return 0;
        }
        ERR("http  dns未查找到%s域名的ip",(char *)hoststr.c_str());
        return -1;
    }
    if (phost == NULL)
    {
        ERR("dns_resolve_114 通过114.114.114.114获取ip地址失败");
        free(buf);
        return -1;
    }

    inet_ntop(AF_INET,(struct in_addr*)hostinfo.h_addr_list[0],
              tmpipaddr,sizeof(tmpipaddr));
    ipaddr = tmpipaddr;
    //DBG("gethostbyname_r获取host:%s,ip地址:%s",
    //         (char *)hoststr.c_str(),tmpipaddr);
    free(buf);
    return 0;
}

/*查询dns解析表:
url:解析url地址;
url_ip:将域名解析成ip的地址;
force:强制解析域名 0,不强制获取，1 强制获取 2,更新dns
*/
static int search_DNS_ipurl(std::string url,std::string &url_ip,
                     int force,std::string &host,std::string &port,std::string &ip_addr)
{
    static pthread_mutex_t dns_table_lock = PTHREAD_MUTEX_INITIALIZER; /*泊位临时数据库线程锁*/
#define DNS_NUMBER   32
    static std::map<std::string,std::string>  dns_table;
    std::string hoststr;
    std::string ipaddr;
    int i;
    int ret;

    if (force == 2)
    {
        //DBG("更新dns");
        pthread_mutex_lock(&dns_table_lock);
        std::map<std::string,std::string>::iterator iter;//定义一个迭代指针iter
        for(iter = dns_table.begin(); iter != dns_table.end(); iter++)
        {
            //DBG("更新dns[%s:%s]",iter->first,iter->second);
            ret = url_get(iter->first,ipaddr);
            if (ret < 0)
            {
                ERR("获取ip地址失败");
            }
            if (strlen(ipaddr.c_str()) > 0)
            {
                iter->second = ipaddr;
            }
        }
        pthread_mutex_unlock(&dns_table_lock);
        return 0;
    }

    if (url.length() == 0 )
    {
        ERR("查询dns错误,url值为空");
        return -1;
    }
    url_ip = url;
    //DBG("查询URL:%s",(char *)url.c_str());
    struct ast_uri* uri_s = ast_uri_parse((char *)url.c_str());
    if(uri_s->host == NULL)
    {
        ERR("查询dns错误,url:%s",url.c_str());
        return -1;
    }
    hoststr = uri_s->host;
    host = uri_s->host;
    if(uri_s->port != NULL)
    {
        port = uri_s->port;
    }else
    {
        port = "80";
    }
    free(uri_s);

    //DBG("host:%s",(char *)hoststr.c_str());
    if (isIP((char *)host.c_str()) == 1)
    {
        DBG("url:%s的host地址是IP不进行解析",(char *)url.c_str());
        return -1;
    }
    pthread_mutex_lock(&dns_table_lock);
    std::map<std::string,std::string>::iterator tmpit;
    switch(force)
    {
        case 0:/*不强制获取*/
            //DBG("查询dns");
            tmpit = dns_table.find(hoststr);
            if (tmpit == dns_table.end())
            {
                ret = url_get(hoststr,ipaddr);
                if (ret < 0)
                {
                    ERR("获取ip地址失败");
                    pthread_mutex_unlock(&dns_table_lock);
                    return -1;
                }
                if (strlen(ipaddr.c_str()) > 0)
                {
                    dns_table[hoststr] = ipaddr;
                }
            }else
            {
                ipaddr = dns_table[hoststr];
            }
            break;
        case 1:/*强制获取*/
            //DBG("强制获取dns");
            ret = url_get(hoststr,ipaddr);
            if (ret < 0)
            {
                ERR("获取ip地址失败");
                pthread_mutex_unlock(&dns_table_lock);
                return -1;
            }
            if (strlen(ipaddr.c_str()) > 0)
            {
                dns_table[hoststr] = ipaddr;
            }
            break;
        default:
            tmpit = dns_table.find(hoststr);
            if (tmpit == dns_table.end())
            {
                ret = url_get(hoststr,ipaddr);
                if (ret < 0)
                {
                    ERR("获取ip地址失败");
                    pthread_mutex_unlock(&dns_table_lock);
                    return -1;
                }
                if (strlen(ipaddr.c_str()) > 0)
                {
                    dns_table[hoststr] = ipaddr;
                }
            }else
            {
                ipaddr = dns_table[hoststr];
            }
            break;
    }
    pthread_mutex_unlock(&dns_table_lock);
    url_ip=url_ip.replace(url_ip.find(hoststr),hoststr.length(),ipaddr);
    if (url_ip.find("http://") != std::string::npos)
    {
        url_ip=url_ip.replace(url_ip.find("http://"),strlen("http://"),"");
    }
    //DBG("返回url_ip=%s",(char *)url_ip.c_str());
    return 0;
}

/*dns服务*/
static void* dns_server_task(void *argv)
{
    while(1)
    {
        std::string tmpurl,tmpip,tmphost,tmpport,tmp_ipaddr;
        search_DNS_ipurl(tmpurl,tmpip,2,tmphost,tmpport,tmp_ipaddr);
        sleep(60*30);/*30分钟进行一次dns的更新*/
    }
    return NULL;
}

/*dns服务*/
int DNSServerStart()
{
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, dns_server_task, NULL);
    if(ret != 0)
    {
        DBG("Can't create dns_server_start()\n");
    }else{
        DBG("Create dns_server_start() succeed");
    }
}