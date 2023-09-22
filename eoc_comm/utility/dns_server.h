#ifndef _DNS_SERVER_H_
#define _DNS_SERVER_H_

/*查询dns解析表:
url:解析url地址;
url_ip:将域名解析成ip的地址;
force:强制解析域名 0,不强制获取，1 强制获取 2,更新dns
*/
int search_DNS_ipurl(std::string url,std::string &url_ip,
               int force,std::string &host,std::string &port,std::string &ip_addr);

int isIP(char *str);
int url_get(std::string hoststr,std::string &ipaddr);
/*dns服务*/
int dns_server_start();

#endif
