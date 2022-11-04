/*
 * blue_tcp.cpp
 *
 *  Created on: 2020年4月26日
 *      Author: zpc
 *  功能描述：
 *      提供一个TCP连接客户端，同时提供周期性回调，连接、数据到来、断开连接回调函数
 */
#include "ttcp.hpp"

#include <stdio.h>
#include <pthread.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <poll.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>

#include <list>
#include <queue>
#include <vector>


#if 1

#include "logger.h"

#else

#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x

#define TIME_STR_LEN    50
#define GET_CURRENT_TIME(p) {time_t now; struct tm *timenow; time(&now); timenow=localtime(&now); sprintf(p, "%u-%02u-%02u %02u:%02u:%02u", 1900+timenow->tm_year, 1+timenow->tm_mon, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);}

#define DBG(fmt, args...)     do{if(1){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]D:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define INFO(fmt, args...)    do{if(1){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]I:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define WARNING(fmt, args...) do{if(1){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]W:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define ERR(fmt, args...)     do{if(1){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]E:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)
#define CRIT(fmt, args...)    do{if(1){char _p[TIME_STR_LEN] = {0}; GET_CURRENT_TIME(_p); printf("[%s][%s-%u]C:" fmt "\n",_p,filename(__FILE__), __LINE__, ##args);}}while(0)

#endif

#if 0
struct tcp_client{
    char name[32];
    char host[32];
    int port;
    connected_callback_t connetced_callback;
    datacoming_callback_t datacoming_callback;
    disconnected_callback_t disconnected_callback;
    interval_callback_t interval_callback;
    int interval_s;

    bool running;
    int sock_fd;

    SSL_CTX *cli_ctx;
    SSL *cli_ssl;
    
    pthread_t id;
    pthread_mutex_t mutex;
};
#endif
#define MAX_CLIENT_CNT 1024
struct tcp_server{
    char name[32];
    char host[32];
    int port;
//  connected_callback_t connetced_callback;
    datacoming_callback1_t datacoming_callback;
//  disconnected_callback_t disconnected_callback;
//  interval_callback_t interval_callback;
//  int interval_s;

    tcp_server_connected_callback_t tcp_server_connected_callback;
    tcp_server_recv_data_callback_t tcp_server_recv_data_callback;
    tcp_server_disconnected_callback_t tcp_server_disconnected_callback;

    bool running;
    int sock_fd;
    pthread_t id;
    pthread_mutex_t mutex;

    void *user_data[MAX_CLIENT_CNT];
    int connfd_list[MAX_CLIENT_CNT];
    time_t recv_data_t[MAX_CLIENT_CNT];
    int connfd_cnt;
    bool need_reset_fds;
};

//tls加密
#define TLS_SERVER_CA_FILE "./eoc.pem"

void print_error_string(unsigned long err, const char* const label)
{
    const char* const str = ERR_reason_error_string(err);
    if(str) {
        ERR("%s", str);
    }else {
        ERR("%s failed: %lu (0x%lx)", label, err, err);
    }
}
void ShowCerts(SSL * ssl)
{
    X509 *cert;
    char *line;
     
    cert = SSL_get_peer_certificate(ssl);
    if (cert != NULL) {
        printf("数字证书信息:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("证书: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("颁发者: %s\n", line);
        free(line);
        X509_free(cert);
    } else
        {
        printf("无证书信息！\n");
        }
}
int verify_callback(int preverify, X509_STORE_CTX* x509_ctx)
{
    /* For error codes, see http://www.openssl.org/docs/apps/verify.html  */
   
    int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
    int err = X509_STORE_CTX_get_error(x509_ctx);
    
    X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);
    X509_NAME* iname = cert ? X509_get_issuer_name(cert) : NULL;
    X509_NAME* sname = cert ? X509_get_subject_name(cert) : NULL;
    
    printf("verify_callback (depth=%d)(preverify=%d)\n", depth, preverify);
#if 0    
    /* Issuer is the authority we trust that warrants nothing useful */
    print_cn_name("Issuer (cn)", iname);
    
    /* Subject is who the certificate is issued to by the authority  */
    print_cn_name("Subject (cn)", sname);
    
    if(depth == 0) {
        /* If depth is 0, its the server's certificate. Print the SANs */
        print_san_name("Subject (san)", cert);
    }
#endif
    if(preverify == 0)
    {
        if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
            printf("  Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n");
        else if(err == X509_V_ERR_CERT_UNTRUSTED)
            printf("  Error = X509_V_ERR_CERT_UNTRUSTED\n");
        else if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
            printf("  Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n");
        else if(err == X509_V_ERR_CERT_NOT_YET_VALID)
            printf("  Error = X509_V_ERR_CERT_NOT_YET_VALID\n");
        else if(err == X509_V_ERR_CERT_HAS_EXPIRED)
            printf("  Error = X509_V_ERR_CERT_HAS_EXPIRED\n");
        else if(err == X509_V_OK)
            printf("  Error = X509_V_OK\n");
        else
            printf("  Error = %d\n", err);
    }

#if !defined(NDEBUG)
    return 1;
#else
    return preverify;
#endif
}

int ssl_creat_info(int sock, SSL_CTX *&ctx, SSL *&ssl)
{
    int ret = 0;
    if(sock <= 0)
    {
        ERR("sock=%d", sock);
        return -1;
    }
//    DBG("111");
    //ssl初始化
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    //建立ctx
    const SSL_METHOD *method;
    method = SSLv23_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR("Unable to create SSL context");
    //    ERR_print_errors_fp(stderr);
        return -1;
    }
//    DBG("222");
    //设置ssl及证书
    /* https://www.openssl.org/docs/ssl/ctx_set_verify.html */
    SSL_CTX_set_verify_depth(ctx, 5);
    /* Cannot fail ??? */
    /* Remove the most egregious. Because SSLv2 and SSLv3 have been      */
    /* removed, a TLSv1.0 handshake is used. The client accepts TLSv1.0  */
    /* and above. An added benefit of TLS 1.0 and above are TLS          */
    /* extensions like Server Name Indicatior (SNI).                     */
    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    long old_opts = SSL_CTX_set_options(ctx, flags);
    SSL_CTX_set_ecdh_auto(ctx, 1);
    
    if(SSL_CTX_load_verify_locations(ctx, TLS_SERVER_CA_FILE, NULL)<=0)
    {
        unsigned long ssl_err = ERR_get_error();
        print_error_string(ssl_err, "SSL_CTX_load_verify_locations");
        return -1;
    }
//    DBG("333");
#if 0
    //建立安全连接双向认证，如需验证客户端添加
    /* Set the key and cert 加载客户端证书和私钥*/
    if (SSL_CTX_use_certificate_file(ctx, "./cert.pem", SSL_FILETYPE_PEM) <= 0) 
    {
    //    ERR_print_errors_fp(stderr);
    //    exit(EXIT_FAILURE);
        ERR("SSL_CTX_use_certificate_file err");
        return -1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "./key.pem", SSL_FILETYPE_PEM) <= 0 ) 
    {
    //    ERR_print_errors_fp(stderr);
    //    exit(EXIT_FAILURE);
        ERR("SSL_CTX_use_PrivateKey_file err");
        return -1;
    }
    //检查私钥是否和证书匹配
    if(!SSL_CTX_check_private_key( ctx ))
    {
        //处理错误
    }
#endif
    /* https://www.openssl.org/docs/ssl/ctx_set_verify.html */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
    /* Cannot fail ??? */
//    DBG("444");
    //建立ssl连接
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
//    DBG("set ok");
    ret = SSL_connect(ssl);
//    DBG("555");
    if(ret <= 0)
    {
        unsigned long err_num = ERR_get_error();
        print_error_string(err_num, "SSL_connect");
        ShowCerts(ssl);
        return -1;
    }
    else
    {
        DBG("ssl connect success, return %d", ret);
        ShowCerts(ssl);
    }
//    DBG("666");
    return 0;
}
#define MAXBUF 1024
int receive_data(int sock_fd, SSL *ssl, string &data)
{
    int ret = 0;
    int timeout = 3*1000;
    fd_set fds;
    int cnt = 0;

    while(1)
    {
        FD_ZERO(&fds);
        FD_SET(sock_fd, &fds);
        struct timeval tv;
        if(timeout >= 1000)
        {
            tv.tv_sec = timeout/1000;
            tv.tv_usec = 1000*(timeout%1000);
        }
        else
        {
            tv.tv_sec = 0;
            tv.tv_usec = 1000*timeout;
        }
        ret = select(sock_fd + 1, &fds, NULL, NULL, &tv);
        if(ret < 0)
        {
            DBG("cloud_sock err");
            DBG("select返回<0, ret=%d, 错误代码是%d，错误信息是'%s'\n", ret, errno, strerror(errno));
            return -1;
        }
        else if(ret == 0)
        {
            //DBG("time out");
            break;
        }
        else
        {
            DBG("sock(%d) ret = %d", sock_fd, ret);
            if(FD_ISSET(sock_fd, &fds))
            {
                do{
                    char buffer[MAXBUF + 1];
                    bzero(buffer, MAXBUF + 1);
                
                    ret = SSL_read(ssl, buffer, MAXBUF);
                    unsigned long errnum = SSL_get_error(ssl, ret);
                    if (ret > 0){
                        cnt += ret;
                        DBG("接收消息成功:'%s'，共%d个字节的数据\n",buffer, ret);
                        data.append(buffer, ret);
                    }
                    else if(ret == 0) {
                        DBG("链接断开, SSL_read返回0");
                        return -1;
                    }
                    else {
                        DBG("消息接收失败！ret=%d, 错误代码是%d，错误信息是'%s'\n", ret, errno, strerror(errno));
                        if(ret == -1)
                        {
                            DBG("重新链接");
                            return -1;
                        }
                        return 0;
                    }
                }while(SSL_pending(ssl) > 0);
                timeout = 10;
            }

        }
    }
    if(cnt > 0)
    {
//        DBG("receive data:%s, num=%d", data.c_str(), cnt);
    }
    return cnt;
}


/*
 * TCP连接
 * */
int tcp_socket_connect(const char* name, const char* ip, int port)
{
    int ret = 0;
    int reuse = 1;
    struct sockaddr_in serv;
    int sock = 0;

    DBG("%s:connecting to cloud server[%s,%s,%d]...", __FUNCTION__, name, ip, port);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        ERR("%s:Can't make socket", __FUNCTION__);
        return -1;
    }

    memset(&serv, 0, sizeof (struct sockaddr_in));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));
    if(ret < 0){
        ERR("%s:can't set sockopt SO_REUSEADDR to socket %d", __FUNCTION__, sock);
        return -1;
    }

    ret = connect(sock, (struct sockaddr *)&serv, sizeof(struct sockaddr_in));
    if (ret < 0){
        close (sock);
        ERR("%s:Can't connect to cloud server[%s,%s,%d]", __FUNCTION__, name, ip, port);
        return -1;
    }
    return sock;
}

static void* tcp_client_mainloop(void *argv)
{
    int ret = 0;
    BLUE_TCP_STATE state = BLUE_TCP_OK;
    tcp_client_t* client = (tcp_client_t*)argv;
    time_t last_run_interval_callback_t = time(NULL);

    client->running = true;
    INFO("TCP client[%s,%s:%d] 0x%x is running", client->name, client->host, client->port, (unsigned int)client->id);
    while(client->running)
    {
        int sock_fd = tcp_socket_connect(client->name, client->host, client->port);
        if(sock_fd >= 0){
        //    pthread_mutex_lock(&client->mutex);
            client->sock_fd = sock_fd;
        //    pthread_mutex_unlock(&client->mutex);
            INFO("TCP client[%s,%s:%d] 0x%x connect sock_fd(%d)", client->name, client->host, client->port,
                    (unsigned int)client->id, client->sock_fd);
            SSL_CTX *ctx;
            SSL *ssl;
            ret = ssl_creat_info(sock_fd, ctx, ssl);
            if(ret == 0)
            {
                //ssl建立连接完成
            //    pthread_mutex_lock(&client->mutex);
                client->cli_ctx = ctx;
                client->cli_ssl = ssl;
            //    pthread_mutex_unlock(&client->mutex);
            #if 0
                int flag;
                flag = fcntl(client->sock_fd, F_GETFL, 0);
                flag |= O_NONBLOCK;
                fcntl(client->sock_fd, F_SETFL, flag); //设置非阻塞
            #endif
                INFO("SSL client[%s,%s:%d] 0x%x connect sock_fd(%d)", client->name, client->host, client->port,
                        (unsigned int)client->id, client->sock_fd);
                
                state = BLUE_TCP_OK;
            //    string test_send = "test tcp_client_write***************";
            //    tcp_client_write(client, test_send.c_str(), test_send.length());
                if(client->connetced_callback){
                    state = client->connetced_callback(client);
                    if(state != BLUE_TCP_OK){
                        ERR("TCP client[%s,%s:%d] 0x%x connetced_callback run is err", client->name, client->host, client->port,
                                (unsigned int)client->id);
                    }
                }
                DBG("callback ok, state = %d", (int)state);
                if(state == BLUE_TCP_OK)
                {
                    int nread = 0;
                    string data;
                    data.clear();
                    while(client->running)
                    { 
                    //    DBG("receive data, datacoming_callback");
                        nread = 0;
                        nread = receive_data(client->sock_fd, client->cli_ssl, data);
                        if(nread < 0)
                        {
                            ERR("tcp connect err");
                            break;
                        }
                        else if(nread > 0)
                        {
                            if(client->datacoming_callback)
                            {
                                state = client->datacoming_callback(client, (char*)data.c_str(), ret);
                                if(state != BLUE_TCP_OK){
                                    ERR("TCP client[%s,%s:%d] 0x%x datacoming_callback is err break", client->name, client->host, client->port,
                                            (unsigned int)client->id);
                                    break;
                                }
                            }
                            data.clear();
                        }
                   //     DBG("interval_callback");
                        if(time(NULL) >= last_run_interval_callback_t + client->interval_s){
                            last_run_interval_callback_t = time(NULL);
                            if(client->interval_callback){
                                state = client->interval_callback(client);
                                if(state != BLUE_TCP_OK){
                                    ERR("TCP client[%s,%s:%d] 0x%x interval_callback is err break", client->name, client->host, client->port,
                                            (unsigned int)client->id);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                //ssl连接失败
                ERR("ssl_creat failed");
            }

            //出错，关闭连接
            ERR("close ssl & socket");
       //     pthread_mutex_lock(&client->mutex);
            EVP_cleanup();
            if(client->cli_ssl)
            {
                SSL_free(client->cli_ssl);
                client->cli_ssl = NULL;
            }
            if(client->cli_ctx)
            {
                SSL_CTX_free(client->cli_ctx);
                client->cli_ctx = NULL;
            }
            
            if(client->sock_fd >= 0){
                INFO("TCP client[%s,%s:%d] 0x%x close sock_fd(%d)", client->name, client->host, client->port,
                        (unsigned int)client->id, client->sock_fd);
                close(client->sock_fd);
                client->sock_fd = -1;
            }
       //     pthread_mutex_unlock(&client->mutex);

            if(client->disconnected_callback){
                client->disconnected_callback();
            }
        }else{
            //连接失败
            ERR("TCP client[%s,%s:%d] 0x%x tcp_socket_connect failed", client->name, client->host, client->port,
                    (unsigned int)client->id);
        }
        
        if(client->running){
            sleep(10);
            INFO("TCP client[%s,%s:%d] 0x%x reconnect", client->name, client->host, client->port,
                    (unsigned int)client->id);
        }
    }
//    pthread_mutex_lock(&client->mutex);
    INFO("TCP client[%s,%s:%d] 0x%x exit", client->name, client->host, client->port,
            (unsigned int)client->id);
    sleep(1);
//    pthread_mutex_unlock(&client->mutex);
    pthread_mutex_destroy(&client->mutex);
    free(client);
    pthread_exit((void *)0);
    return (void *)0;
}
/*
 * 创建一个TCP client自动维护连接、断开、数据接收、发送
 *
 * 参数：
 * name：TCP的可打印名称
 * host：IP地址或域名
 * port：端口号
 * connetced_callback：连接成功回调函数，在回调函数里面可以通过tcp_client_write发送数据
 * datacoming_callback：数据接收回调函数，在回调函数里面可以通过tcp_client_write发送数据
 * disconnected_callback：断开连接回调函数，在回调函数里面可以通过tcp_client_write发送数据
 * interval_callback：周期回调函数，在回调函数里面可以通过tcp_client_write发送数据
 * interval_s：周期回调函数的回调周期（不严格的时间）
 *
 * 返回值：
 * NULL：创建失败
 * 非NULL：创建TCP client成功，返回句柄
 * */
tcp_client_t* tcp_client_create(const char* name, const char* host, int port, connected_callback_t connetced_callback,
        datacoming_callback_t datacoming_callback, disconnected_callback_t disconnected_callback,
        interval_callback_t interval_callback, int interval_s)
{
    tcp_client_t* client = (tcp_client_t*)calloc(sizeof(tcp_client_t), 1);
    if(client){
        snprintf(client->name, sizeof(client->name), "%s", name);
        snprintf(client->host, sizeof(client->host), "%s", host);
        client->port = port;
        client->connetced_callback = connetced_callback;
        client->datacoming_callback = datacoming_callback;
        client->disconnected_callback = disconnected_callback;
        client->interval_callback = interval_callback;
        client->interval_s = interval_s;
        client->sock_fd = -1;
        client->cli_ctx = NULL;
        client->cli_ssl = NULL;

        if(pthread_mutex_init(&client->mutex, NULL) == 0){
            if(pthread_create(&client->id, NULL, tcp_client_mainloop, (void*)client) == 0){
                INFO("%s tcp_client[%s,%s:%d] create tcp_client_mainloop succeed", __FUNCTION__, name, host, port);
                return client;
            }else{
                pthread_mutex_destroy(&client->mutex);
                free(client);
                ERR("%s tcp_client[%s,%s:%d] pthread_create err", __FUNCTION__, name, host, port);
                return NULL;
            }

        }else{
            ERR("%s tcp_client[%s,%s:%d] pthread_mutex_init err", __FUNCTION__, name, host, port);
            free(client);
            return NULL;
        }
    }else{
        ERR("%s tcp_client[%s,%s:%d] calloc err", __FUNCTION__, name, host, port);
        return NULL;
    }
    return NULL;
}
/*
 * 发送指定字节数的数据
 *
 * 参数：
 * client： tcp_client_create接口返回的句柄
 * data：发送的数据
 * data_size：发送的数据长度
 *
 * 返回值：
 * -1：发送失败（socket出错）
 * >=0：发送出去的字节数
 * */
int tcp_client_write(tcp_client_t* client, const char* data, size_t data_size)
{
    pthread_mutex_lock(&client->mutex);
    if(client->sock_fd){
        int nleft = 0;
        int nsend = 0;
        const char *ptr = NULL;
        //DBG("send:%s", data);
        ptr = data;
        nleft = data_size;
        while(nleft > 0){
            if((nsend = SSL_write(client->cli_ssl, ptr, nleft)) < 0){
                if(errno == EINTR){
                    DBG("errno = EINTR");
                    nsend = 0;          /* and call send() again */
                }else{
                    ERR("消息data='%s' nsend=%d, 错误代码是%d, 错误信息是'%s'", data, nsend, errno, strerror(errno));
                    pthread_mutex_unlock(&client->mutex);
                    return(-1);
                }
            }else if(nsend == 0){
                break;                  /* EOF */
            }
            nleft -= nsend;
            ptr += nsend;
        }
        pthread_mutex_unlock(&client->mutex);
        return(data_size-nleft);              /* return >= 0 */
    }else{
        pthread_mutex_unlock(&client->mutex);
        return -1;
    }
}

void tcp_client_stop(tcp_client_t* client)
{
    client->running = false;
}
/*
 * TCP Server *******************************************************************************************************************************
 * */
/*
 * 发送指定字节数的数据
 *
 * 参数：
 * client： tcp_client_create接口返回的句柄
 * data：发送的数据
 * data_size：发送的数据长度
 *
 * 返回值：
 * -1：发送失败（socket出错）
 * >=0：发送出去的字节数
 * */
int tcp_server_write(int connfd, const char* data, size_t data_size)
{
    if(connfd){
        int nleft = 0;
        int nsend = 0;
        const char *ptr = NULL;

        ptr = data;
        nleft = data_size;
        while(nleft > 0){
            if((nsend = send(connfd, ptr, nleft, 0)) < 0){
                if(errno == EINTR){
                    nsend = 0;          /* and call send() again */
                }else{
                    return(-1);
                }
            }else if(nsend == 0){
                break;                  /* EOF */
            }
            nleft -= nsend;
            ptr += nsend;
        }
        return(data_size-nleft);              /* return >= 0 */
    }else{
        return -1;
    }
}

void tcp_server_stop(tcp_server_t* server)
{
    server->running = false;
}
static void tcp_server_client_init(tcp_server_t* server)
{
    for(int i=0; i<MAX_CLIENT_CNT; i++){
        server->connfd_list[i] = -1;
        server->user_data[i] = NULL;
        server->recv_data_t[i] = 0;
    }
}
static void tcp_server_client_add(tcp_server_t* server, int connfd)
{
    if(server->connfd_cnt < MAX_CLIENT_CNT){
        // 添加新的连接
        for(int i=0; i<MAX_CLIENT_CNT; i++){
            if(server->connfd_list[i] == -1){
                if(server->tcp_server_connected_callback){
                    if(server->tcp_server_connected_callback(connfd, &(server->user_data[i])) == 0){
                        server->recv_data_t[i] = time(NULL);
                        server->connfd_list[i] = connfd;
                        server->connfd_cnt++;
                        server->need_reset_fds = true;
                        DBG("%s:set connfd_list[%d]=%d connfd_cnt=%d", __FUNCTION__, i, server->connfd_list[i], server->connfd_cnt);
                    }else{
                        ERR("%s:set connfd_list[%d]=%d tcp_server_connected_callback Err, close client", __FUNCTION__, i, connfd);
                        close(connfd);
                    }
                }else{
                    server->recv_data_t[i] = time(NULL);
                    server->connfd_list[i] = connfd;
                    server->connfd_cnt++;
                    server->need_reset_fds = true;
                    DBG("%s:set connfd_list[%d]=%d connfd_cnt=%d", __FUNCTION__, i, server->connfd_list[i], server->connfd_cnt);
                }
                break;
            }
        }
    }else{
        // 已达到最大连接数
        INFO("%s:connfd_cnt is MAX_CLIENT_CNT[%d], close client", __FUNCTION__, MAX_CLIENT_CNT);
        close(connfd);
    }
}
static void tcp_server_client_del(tcp_server_t* server, int connfd)
{
    for(int i=0; i<MAX_CLIENT_CNT; i++){
        if(server->connfd_list[i] == connfd){
            if(server->tcp_server_disconnected_callback){
                server->tcp_server_disconnected_callback(server->connfd_list[i], server->user_data[i]);
            }
            close(server->connfd_list[i]);
            server->recv_data_t[i] = 0;
            server->connfd_list[i] = -1;
            server->connfd_cnt--;
            server->need_reset_fds = true;
            DBG("%s:set connfd_list[%d]=%d connfd_cnt=%d", __FUNCTION__, i, server->connfd_list[i], server->connfd_cnt);
            break;
        }
    }
}
static void* tcp_server_mainloop(void *argv)
{
    tcp_server_t* server = (tcp_server_t *)argv;
    server->running = true;
    INFO("TCP server[%s,%s:%d] 0x%x is running", server->name, server->host, server->port, (unsigned int)server->id);

    while(server->running){
        int ret = 0;
        int cnt = 0;
        int nread = 0;
        struct pollfd fds[MAX_CLIENT_CNT + 1];
        /*
         * 初始化所有客户端
         * */
        tcp_server_client_init(server);

        /*
         * 添加服务器sock的监听
         * */
        fds[0].fd = server->sock_fd;
        fds[0].events = POLLIN;
        while(server->running){
            // 添加需要监听的fd
            if(server->need_reset_fds){
                int index = 1;
                server->need_reset_fds = false;
                time_t now = time(NULL);
                for(int i=0; i<MAX_CLIENT_CNT; i++){
                    if(server->connfd_list[i] != -1){
                        if(now - server->recv_data_t[i] > 5*60){
                            if(server->tcp_server_disconnected_callback){
                                server->tcp_server_disconnected_callback(server->connfd_list[i], server->user_data[i]);
                            }
                            close(server->connfd_list[i]);
                            server->connfd_list[i] = -1;
                            server->connfd_cnt--;
                            DBG("%s:set connfd_list[%d]=%d connfd_cnt=%d too long time no data", __FUNCTION__, i, server->connfd_list[i], server->connfd_cnt);
                        }else{
                            fds[index].fd = server->connfd_list[i];
                            fds[index].events = POLLIN;
                            fds[index].revents = 0;
                            DBG("%s:add connfd_list[%d][%d][t=%ld] to POLL fds[%d]", __FUNCTION__, i, server->connfd_list[i], server->recv_data_t[i], index);
                            index++;
                        }
                    }
                }
            }
            // 监听
            cnt = poll(fds, 1 + server->connfd_cnt, 1000);
            if(cnt == -1){
                //监听出错
                ERR("TCP server[%s,%s:%d] 0x%x poll err break", server->name, server->host, server->port,
                        (unsigned int)server->id);
                break;
            }else if(cnt == 0){
                //超时，不需要处理
            }else{
                //监听fd处理
                for(int fds_i=0; fds_i<server->connfd_cnt+1; fds_i++){
                    if(fds_i == 0){
                        /*
                         * 处理客户端连接
                         * */
                        if(fds[fds_i].revents & POLLIN){
                            fds[fds_i].revents = 0;
                            int connfd = -1;
                            if ((connfd = accept(server->sock_fd, (struct sockaddr *)NULL, NULL)) == -1) {
                                server->running = false;
                                ERR("%s:accept socket[%d] error: %s(errno: %d)", __FUNCTION__, server->sock_fd, strerror(errno), errno);
                                break;
                            }
                            DBG("%s:accept socket[%d] connfd=%d", __FUNCTION__, server->sock_fd, connfd);
                            tcp_server_client_add(server, connfd);
                        }
                    }else{
                        // 处理客户端
                        if(fds[fds_i].revents & POLLIN){
                            fds[fds_i].revents = 0;
                            ioctl(fds[fds_i].fd, FIONREAD, &nread);
                            if(nread <= 0){
                                DBG("TCP server[%s,%s:%d] 0x%x connfd[%d] is closed", server->name, server->host, server->port,
                                        (unsigned int)server->id, fds[fds_i].fd);
                                tcp_server_client_del(server, fds[fds_i].fd);
                            }else{
                                char tmp[1024] = {0};
                                nread = nread<1024?nread:1024;
                                ret = recv(fds[fds_i].fd, tmp, nread, 0);
                                if (ret > 0){
                                    if(server->tcp_server_recv_data_callback){
                                        for(int i=0; i<MAX_CLIENT_CNT; i++){
                                            if(server->connfd_list[i] == fds[fds_i].fd){
                                                server->recv_data_t[i] = time(NULL);
                                                if(server->tcp_server_recv_data_callback(server->connfd_list[i], server->user_data[i], tmp, nread) != 0){
                                                    if(server->tcp_server_disconnected_callback){
                                                        server->tcp_server_disconnected_callback(server->connfd_list[i], server->user_data[i]);
                                                    }
                                                    close(server->connfd_list[i]);
                                                    server->connfd_list[i] = -1;
                                                    server->connfd_cnt--;
                                                    server->need_reset_fds = true;
                                                    DBG("%s:set connfd_list[%d]=%d connfd_cnt=%d tcp_server_recv_data_callback Err", __FUNCTION__, i, server->connfd_list[i], server->connfd_cnt);
                                                }
                                                break;
                                            }
                                        }
                                    }else{
#if 0
                                        DBG("TCP server[%s,%s:%d] 0x%x connfd [%d] recv:%s,%d", server->name, server->host, server->port,
                                                (unsigned int)server->id, fds[fds_i].fd, tmp, nread);
                                        tcp_server_write(fds[fds_i].fd, "Hello World", 11);
#endif
                                    }
                                }else{
                                    DBG("TCP server[%s,%s:%d] 0x%x connfd[%d] Recv Err", server->name, server->host, server->port,
                                            (unsigned int)server->id, fds[fds_i].fd);
                                    tcp_server_client_del(server, fds[fds_i].fd);
                                }
                            }
                        }
                    }
                }
            }
        }
        /*
         * 关闭所有客户端
         * */
        DBG("%s:close all client socket", __FUNCTION__);
        for(int i=0; i<MAX_CLIENT_CNT; i++){
            if(server->tcp_server_disconnected_callback){
                server->tcp_server_disconnected_callback(server->connfd_list[i], server->user_data[i]);
            }
            close(server->connfd_list[i]);
            server->user_data[i] = NULL;
            server->connfd_list[i] = -1;
        }
    }

    pthread_mutex_lock(&server->mutex);
    INFO("TCP server[%s,%s:%d] 0x%x exit", server->name, server->host, server->port,
            (unsigned int)server->id);
    sleep(1);
    close(server->sock_fd);
    pthread_mutex_unlock(&server->mutex);
    pthread_mutex_destroy(&server->mutex);
    free(server);
    pthread_exit((void *)0);
    return (void *)0;
}

/*
 * TCP Server 创建服务器
 *
 * 参数：
 * name：服务器名称
 * host：服务器地址
 * port：服务器端口号
 * tcp_server_connected_callback：
 * tcp_server_recv_data_callback：
 * tcp_server_disconnected_callback：
 *
 * 返回值：
 * 失败返回NULL，成功返回TCP Server句柄
 * */
tcp_server_t *tcp_server_create(const char* name, const char* host, int port,
        tcp_server_connected_callback_t tcp_server_connected_callback,
        tcp_server_recv_data_callback_t tcp_server_recv_data_callback,
        tcp_server_disconnected_callback_t tcp_server_disconnected_callback)
{
    tcp_server_t* server = (tcp_server_t*)calloc(1, sizeof(tcp_server_t));
    if(server){
        snprintf(server->name, sizeof(server->name), "%s", name);
        snprintf(server->host, sizeof(server->host), "%s", host);
        server->port = port;
        server->sock_fd = -1;

        server->tcp_server_connected_callback = tcp_server_connected_callback;
        server->tcp_server_recv_data_callback = tcp_server_recv_data_callback;
        server->tcp_server_disconnected_callback = tcp_server_disconnected_callback;

        {
            /*
             * 创建server
             * */
            int ret = 0;
            int reuse = 1;
            struct sockaddr_in serv;
            int sock = 0;

            DBG("%s:start server[%s,%s,%d]...", __FUNCTION__, server->name, server->host, server->port);
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if(sock < 0){
                ERR("%s:Can't make socket", __FUNCTION__);
                free(server);
                return NULL;
            }

            memset(&serv, 0, sizeof (struct sockaddr_in));
            serv.sin_family = AF_INET;
            serv.sin_port = htons(server->port);
            inet_pton(AF_INET, server->host, &serv.sin_addr);

            ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));
            if(ret < 0){
                close(sock);
                ERR("%s:can't set sockopt SO_REUSEADDR to socket %d", __FUNCTION__, sock);
                free(server);
                return NULL;
            }
            if(bind(sock, (struct sockaddr *)&serv, sizeof(serv)) == -1){
                close(sock);
                ERR("%s:bind socket[%d] error: %s(errno: %d)", __FUNCTION__, sock, strerror(errno), errno);
                free(server);
                return NULL;
            }

            if(listen(sock, 10) == -1) {
                close(sock);
                ERR("%s:listen socket[%d] error: %s(errno: %d)", __FUNCTION__, sock, strerror(errno), errno);
                free(server);
                return NULL;
            }
            /*
             * 创建成功
             * */
            server->sock_fd = sock;
        }

        if(pthread_mutex_init(&server->mutex, NULL) == 0){
            if(pthread_create(&server->id, NULL, tcp_server_mainloop, (void*)server) == 0){
                INFO("%s tcp_server[%s,%s:%d] create tcp_server_mainloop succeed", __FUNCTION__, name, host, port);
                return server;
            }else{
                close(server->sock_fd);
                pthread_mutex_destroy(&server->mutex);
                free(server);
                ERR("%s tcp_server[%s,%s:%d] pthread_create err", __FUNCTION__, name, host, port);
                return NULL;
            }
        }else{
            ERR("%s tcp_server[%s,%s:%d] pthread_mutex_init err", __FUNCTION__, name, host, port);
            close(server->sock_fd);
            free(server);
            return NULL;
        }
    }else{
        ERR("%s tcp_server[%s,%s:%d] calloc err", __FUNCTION__, name, host, port);
        return NULL;
    }
    return NULL;
}
#if 0
static BLUE_TCP_STATE connected_callback(tcp_client_t* client)
{
    DBG("%s connected_callback", client->name);
    tcp_client_write(client, "Hello ttcp", 10);
    return BLUE_TCP_OK;
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
static BLUE_TCP_STATE datacoming_callback(tcp_client_t* client, char* data, size_t data_size)
{
    DBG("%s datacoming_callback:%s,%ld", client->name, data, data_size);
    return BLUE_TCP_OK;
}
/*
 * TCP断开连接后回调函数
 *
 * 参数：无
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
static BLUE_TCP_STATE disconnected_callback()
{
    DBG("disconnected_callback");
    return BLUE_TCP_OK;
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
static BLUE_TCP_STATE interval_callback(tcp_client_t* client)
{
//  int ret = tcp_client_write(client, "{}*", 3);
//  DBG("%s interval_callback,ret=%d", client->name, ret);
    tcp_client_write(client, "Hello ttcp", 10);
    return BLUE_TCP_OK;
}
int tcp_client_test()
{
    DBG("tcp_client_test");
//  tcp_client_t* client = tcp_client_create("test", "59.110.92.67", 6226, NULL, datacoming_callback, NULL, interval_callback, 1);
//  tcp_client_t* client = tcp_client_create("TCP Client Test", "127.0.0.1", 5000,
//          connected_callback, datacoming_callback, disconnected_callback, interval_callback, 1);
//  client = tcp_client_create("test2", "59.110.92.67", 6226, NULL, datacoming_callback, NULL, interval_callback, 1);

//  broadcaster_test();
//  while(1){
//      sleep(1);
//  }
    tcp_client_t *client[10] = {0};
    for(int i=0;i<10;i++){
        client[i] = tcp_client_create("TCP Client Test", "127.0.0.1", 5000,
                connected_callback, datacoming_callback, disconnected_callback, interval_callback, 1);
    }

    for(int i=0;i<10;i++){
        sleep(1);
    }

    for(int i=0;i<10;i++){
        tcp_client_stop(client[i]);
    }

    sleep(1);
}
#endif
