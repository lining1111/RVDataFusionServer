/*
 * blue_tcp.hpp
 *
 *  Created on: 2020年4月26日
 *      Author: zpc
 */

#ifndef SOURCE_SRC_BLUEBRAIN2_BLUE_TCP_HPP_
#define SOURCE_SRC_BLUEBRAIN2_BLUE_TCP_HPP_

#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>

using namespace std;

typedef struct tcp_client tcp_client_t;
typedef struct tcp_server tcp_server_t;

typedef enum{
	BLUE_TCP_OK = 0,			//处理成功
	BLUE_TCP_ERR,				//处理失败
}BLUE_TCP_STATE;

/*
 * TCP连接已经建立回调函数
 *
 * 参数：
 * client：连接的socket
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
typedef BLUE_TCP_STATE (*connected_callback_t)(tcp_client_t* client);
/*
 * TCP有数据进来回调函数
 *
 * 参数：
 * client：连接的socket
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
typedef BLUE_TCP_STATE (*datacoming_callback_t)(tcp_client_t* client, char* data, size_t data_size);
typedef BLUE_TCP_STATE (*datacoming_callback1_t)(int connfd, char* data, size_t data_size);
/*
 * TCP断开连接后回调函数
 *
 * 参数：无
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
typedef BLUE_TCP_STATE (*disconnected_callback_t)();

/*
 * TCP连接
 * */
int tcp_socket_connect(const char* name, const char* ip, int port);
int ssl_creat_info(int sock, SSL_CTX *&ctx, SSL *&ssl);
int receive_data(int sock_fd, SSL *ssl, string &data);


/*
 * 周期回调函数
 *
 * 参数：
 * fd：连接的socket fd
 *
 * 返回值：
 * BLUE_TCP_STATE
 * */
typedef BLUE_TCP_STATE (*interval_callback_t)(tcp_client_t* client);

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
		interval_callback_t interval_callback, int interval_s);
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
int tcp_client_write(tcp_client_t* client, const char* data, size_t data_size);


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


/*
 * TCP Server *******************************************************************************************************************************
 * */

/*
 * TCP Server 客户端连接成功后回调函数
 *
 * 参数：
 * connfd：客户端fd
 * user_data：供回调函数初始化的用户数据
 *
 * 返回值：
 * 0：成功
 * -1：失败，会关掉此客户端
 * */
typedef int (*tcp_server_connected_callback_t)(int connfd, void **user_data);

/*
 * TCP Server收到数据后的回调函数
 *
 * 参数：
 * connfd：客户端fd
 * user_data：tcp_server_connected_callback_t里面初始化的用户数据
 * data：收到的数据
 * data_size：收到的数据大小
 *
 * 返回值：
 * 0：成功
 * -1：失败，会关掉此客户端
 * */
typedef int (*tcp_server_recv_data_callback_t)(int connfd, void *user_data, char* data, size_t data_size);

/*
 * TCP Server 客户端断开连接回调函数
 *
 * 参数：
 * connfd：客户端fd
 * user_data：tcp_server_connected_callback_t里面初始化的用户数据
 *
 * 返回值：
 * 0：成功
 * -1：失败，会关掉此客户端
 * */
typedef int (*tcp_server_disconnected_callback_t)(int connfd, void *user_data);

/*
 * TCP Server 像客户端发送数据
 *
 * 参数：
 * connfd：客户端fd
 * data：发送的数据
 * data_size：发送的数据大小
 *
 * 返回值：
 * >=0：发送的数据大小
 * -1：失败
 * */
int tcp_server_write(int connfd, const char* data, size_t data_size);

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
		tcp_server_disconnected_callback_t tcp_server_disconnected_callback);

/*
 * TCP Server 停止
 *
 * 参数：
 * server：tcp_server_create返回的句柄
 *
 * 返回值：无
 * */
void tcp_server_stop(tcp_server_t *server);

int tcp_client_test();

#endif /* SOURCE_SRC_BLUEBRAIN2_BLUE_TCP_HPP_ */
