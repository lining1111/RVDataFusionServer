#include <stdio.h>
#include <stdlib.h>
#include "dns_struct.h"
#include <time.h>  
#include <string.h> 
#include <arpa/inet.h>
#include "logger.h"
#include <vector>
#include <string>

/*

01  00  ID
81  80  flags
00  01  问题数
00  01  应答数
00  00  授权资源记录数
00  01  附加资源记录数
查询区域：
查询名 fs.aps.aipark.com
02  66  73  fs
03  61  70  73  aps
06  61  69  70  61  72  6B  aipark
03  63  6F  6D  com
00  查询名结束
00  01  查询类型
00  01  查询类
回答区域：
应答资源记录（可以多条，每条都是以C0 0C 00 01 00 01开头）
C0  0C  域名 当报文中域名重复出现的时候，该字段使用2个字节的偏移指针来表示，例子C00C(1100000000001100，12正好是头部的长度，其正好指向Queries区域的查询名字字段)
00  01  查询类型 固定2个字节
00  01  查询类 固定2个字节
00  00  00  10 生存时间TTL  查询类型为1时，为4个字节
00  04  资源数据长度 固定2个字节
2F  5F  32  23  47.95.50.35
授权资源记录                             c  a  i   s   h   i   k   0   u  菜市口
09  63  61  69  73  68  69  6B  6F  75  （99 97 105 115 104 105 107 111 117） 
08  72  65  64  69  72  65  63  74  (r e d i r e c t) 
00  
附加资源记录
00  01  查询类型 
00  01  查询类
00  00  0E  10  生存时间TTL
00  04  资源数据长度
7F  00  00  01  127.0.0.1           

*/

/*dns内容信息解析*/
int dns_data_parse(char *dns_buf,int dns_buf_len,std::vector<std::string> &dns_ip_str_all)
{
    int ret = 0;
    char printf_buf[1024];

    memset(printf_buf,0x0,sizeof(printf_buf));

    for(int i = 0;i < dns_buf_len;i++)
    {
        if (strlen(printf_buf) < sizeof(printf_buf) - 8)
        {
            sprintf(printf_buf + strlen(printf_buf),"%02X ",dns_buf[i]&0xFF);
        }
        char *p_dns_buf = dns_buf + i;
        DNS_REQ_DATA *p_dns_head = (DNS_REQ_DATA *)p_dns_buf;
        if (*(unsigned short *)p_dns_head->data_type == 0x0100 && 
                 *(unsigned short *)p_dns_head->data_class == 0x0100)
        {
            if (*(unsigned short *)p_dns_head->data_len == 0x0400)
            {
                char tmp_ip_buf[32];
                memset(tmp_ip_buf,0x0,sizeof(tmp_ip_buf));
                sprintf(tmp_ip_buf,"%u.%u.%u.%u",p_dns_head->data_buf[0],
                            p_dns_head->data_buf[1],p_dns_head->data_buf[2],
                            p_dns_head->data_buf[3]);
                dns_ip_str_all.push_back(tmp_ip_buf);
            }else
            {
                ERR("数据长度错误%u",*(unsigned short *)p_dns_head->data_len);
            }
        }
    }
    DBG("接收数据:%s",printf_buf);
    return 0;
}

/*dns头部信息解析*/
int dns_head_parse(char *dns_buf,int dns_buf_len)
{
    int ret = 0;

    DNS_REQ_HEAD *p_dns_head = (DNS_REQ_HEAD *)dns_buf;
    if (*(unsigned short *)p_dns_head->id != 0x0001)
    {
        ERR("dns解析错误%u",*(unsigned short *)p_dns_head->id);
        return -1;
    }

    if (*(unsigned short *)p_dns_head->flag != 0x8081)
    {
        ERR("dns解析错误%u",*(unsigned short *)p_dns_head->flag);
        return -1;
    }

    return 0;
}

/*dns解析*/
int dns_parse(char *dns_buf,int dns_buf_len,std::vector<std::string> &dns_ip_str_all)
{
    int ret = 0;

    /*头部信息解析*/
    ret = dns_head_parse(dns_buf,dns_buf_len); 
    if (ret < 0)
    {
        ERR("dns解析失败");
        return -1;
    }

    /*dns内容信息解析*/
    ret = dns_data_parse(dns_buf,dns_buf_len,dns_ip_str_all);
    if (ret < 0)
    {
        ERR("dns内容信息解析失败");
        return -1;
    }

    return 0;
}
