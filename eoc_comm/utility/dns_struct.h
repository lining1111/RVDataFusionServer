#ifndef _DNS_STRUCT_H_
#define _DNS_STRUCT_H_
#include <vector>
#include <string>
/*数据包头*/
typedef struct 
{
    unsigned char id[2];   /*会话ID---0表示请求,01 00表示接收*/
    unsigned char flag[2]; /*标志:0x81,0x80表示接收成功*/
    unsigned char ques_num[2];   /*询问信息个数*/
    unsigned char ans_num[2];    /*应答信息个数*/
}DNS_REQ_HEAD;

/*数据报内容*/
typedef struct
{
    unsigned char data_name[2];   /*域名:0xC0,0x0C*/
    unsigned char data_type[2];   /*数据类型1:由域名获得IPv4地址*/
    unsigned char data_class[2];  /*查询类CLASS 1:IN*/
    unsigned char data_time[4];   /*生存时间（TTL）*/ 
    unsigned char data_len[2];    /*数据长度*/
    unsigned char data_buf[4];    /*ipV4的ip地址信息*/
}DNS_REQ_DATA;

/*dns解析*/
int dns_parse(char *dns_buf,int dns_buf_len,std::vector<std::string> &dns_ip_str_all);


#endif
