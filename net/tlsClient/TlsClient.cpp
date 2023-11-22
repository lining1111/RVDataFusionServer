//
// Created by lining on 9/13/22.
//

#include <csignal>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <net/com.h>
#include <glog/logging.h>
#include <sys/ioctl.h>
#include "TlsClient.h"

TlsClient::TlsClient(string ip, int port, string caPath) {
    serverIp = ip;
    serverPort = port;
    serverCAPath = caPath;
    isLive.store(false);
}

TlsClient::~TlsClient() {
}

int TlsClient::connectServer() {
    if (sock < 0) {
        return -1;
    }
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(serverIp.c_str());
    server.sin_port = htons(serverPort);

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //连接前设置非阻塞
    ioctl(sock,FIONBIO,1);
    int ret = connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr));
    ioctl(sock,FIONBIO,0);
    if (ret < 0) {
        close(sock);
        LOG(ERROR) << "tls connect server failed:" << serverIp << ":" << serverPort;
        return -1;
    }
    LOG(WARNING) << "tls connect server success:" << serverIp << ":" << serverPort;
    struct timeval tv;
    tv.tv_sec  = 30;
    tv.tv_usec = 0;
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
    if (ret == -1) {
        close(sock);
        return -1;
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
    if (ret == -1) {
        close(sock);
        return -1;
    }

    return 0;
}

void TlsClient::printErrorString(unsigned long err, const char *const label) {
    const char *const str = ERR_reason_error_string(err);
    if (str) {
        printf("%s\n", str);
        LOG(ERROR) << str;
    } else {
        printf("%s failed: %lu (0x%lx)\n", label, err, err);
        LOG(ERROR) << label << " failed: " << err << " (0x" << err << ")";
    }
}

int TlsClient::verifyCB(int preVerify, X509_STORE_CTX *x509_ctx) {
    /* For error codes, see http://www.openssl.org/docs/apps/verify.html  */

    int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
    int err = X509_STORE_CTX_get_error(x509_ctx);

    X509 *cert = X509_STORE_CTX_get_current_cert(x509_ctx);
    X509_NAME *iname = cert ? X509_get_issuer_name(cert) : NULL;
    X509_NAME *sname = cert ? X509_get_subject_name(cert) : NULL;

    printf("verify_callback (depth=%d)(preVerify=%d)\n", depth, preVerify);
#if 0
    /* Issuer is the authority we trust that warrants nothing useful */
    print_cn_name("Issuer (cn)", iname);

    /* Subject is who the certificate is issued to by the authority  */
    print_cn_name("Subject (cn)", sname);

    if(depth == 0) {
        /* If depth is 0, its the Server's certificate. Print the SANs */
        print_san_name("Subject (san)", cert);
    }
#endif
    if (preVerify == 0) {
        if (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
            printf("  Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n");
        else if (err == X509_V_ERR_CERT_UNTRUSTED)
            printf("  Error = X509_V_ERR_CERT_UNTRUSTED\n");
        else if (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
            printf("  Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n");
        else if (err == X509_V_ERR_CERT_NOT_YET_VALID)
            printf("  Error = X509_V_ERR_CERT_NOT_YET_VALID\n");
        else if (err == X509_V_ERR_CERT_HAS_EXPIRED)
            printf("  Error = X509_V_ERR_CERT_HAS_EXPIRED\n");
        else if (err == X509_V_OK)
            printf("  Error = X509_V_OK\n");
        else
            printf("  Error = %d\n", err);
    }

#if !defined(NDEBUG)
    return 1;
#else
    return preVerify;
#endif
}

void TlsClient::showCerts(SSL *ssl) {
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
    } else {
        printf("无证书信息！\n");
    }
}

int TlsClient::ThreadDump(void *pClient) {
    if (pClient == nullptr) {
        return -1;
    }

    auto client = (TlsClient *) pClient;

    fd_set fds;
    printf("client-%d %s run\n", client->sock, __FUNCTION__);
    uint8_t *buffer = new uint8_t[1024 * 1024];

    while (client->isLive.load()) {
        int len = 0;
        if (client->isAsynRecieve) {

            FD_ZERO(&fds);
            FD_SET(client->sock, &fds);
            struct timeval tv = {
                    3, 0
            };
            int ret = select(client->sock + 1, &fds, nullptr, nullptr, &tv);
            if (ret < 0) {
//                client->isLive.store(false);
            } else if (ret == 0) {
                // time out
            } else {
                if (FD_ISSET(client->sock, &fds)) {
                    bzero(buffer, ARRAY_SIZE(buffer));
                    int recvLen = (client->rb->GetWriteLen() < ARRAY_SIZE(buffer)) ? client->rb->GetWriteLen()
                                                                                   : ARRAY_SIZE(buffer);

                    len = SSL_read(client->ssl, buffer, recvLen);
                    if (len > 0) {
                        //将数据存入缓存
                        if (client->rb != nullptr) {
                            client->rb->Write(buffer, len);
                        }
                    } else if (len == 0) {
                        //断开链接
//                        client->isLive.store(false);
                    } else {
                        if (len < 0) {
                            printf("recv sock %d err:%s\n", client->sock, strerror(errno));
                            //向服务端抛出应该关闭
                            client->isLive.store(false);
                        }
                    }
                }
            }
        }
        usleep(10);
    }
    delete[] buffer;
    printf("client-%d %s exit\n", client->sock, __FUNCTION__);
    return 0;
}

int TlsClient::Open() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG(ERROR) << "tls sock err";
        return -1;
    }
    if (connectServer() < 0) {
        LOG(ERROR) << "tls connect server err";
        return -1;
    }
    int ret;
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_CRYPTO_strings();
    OpenSSL_add_ssl_algorithms();
    //建立ctx
    const SSL_METHOD *method;
    method = SSLv23_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        LOG(ERROR) << "tls unable to create ssl ctx";
        return -1;
    }
    //设置ssl及证书
    SSL_CTX_set_verify_depth(ctx, 5);
    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    long oldOPts = SSL_CTX_set_options(ctx, flags);
    SSL_CTX_set_ecdh_auto(ctx, 1);

    if (SSL_CTX_load_verify_locations(ctx, serverCAPath.c_str(), nullptr) <= 0) {
        unsigned long sslErr = ERR_get_error();
        printErrorString(sslErr, "SSL_CTX_load_verify_locations");
        return -1;
    }
#if 0
    //建立安全连接双向认证，如需验证客户端添加
    /* Set the key and cert 加载客户端证书和私钥*/
    if (SSL_CTX_use_certificate_file(ctx, "./cert.pem", SSL_FILETYPE_PEM) <= 0)
    {

        Error("SSL_CTX_use_certificate_file err");
        return -1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "./key.pem", SSL_FILETYPE_PEM) <= 0 )
    {;
        Error("SSL_CTX_use_PrivateKey_file err");
        return -1;
    }
    //检查私钥是否和证书匹配
    if(!SSL_CTX_check_private_key( ctx ))
    {
        //处理错误
    }
#endif
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verifyCB);
    //建立ssl连接
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    ret = SSL_connect(ssl);
    if (ret <= 0) {
        unsigned long sslErr = ERR_get_error();
        printErrorString(sslErr, "SSL_CTX_load_verify_locations");
        showCerts(ssl);
        return -1;
    } else {
        showCerts(ssl);
    }
    rb = new RingBuffer(1024 * 1024 * 4);
    return 0;
}

int TlsClient::Run() {
    isLive.store(true);

    isLocalThreadRun = true;
    //dump
    futureDump = std::async(std::launch::async, ThreadDump, this);
    return 0;
}

int TlsClient::Close() {
    if (isLive.load() == true) {
        isLive.store(false);
    }
    if (sock > 0) {
        cout << "关闭sock：" << to_string(sock) << endl;
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    if (isLocalThreadRun) {
        try {
            futureDump.wait();
        } catch (std::future_error e) {
            printf("%s %s\n", __FUNCTION__, e.what());
        }
    }
    isLocalThreadRun = false;

    if (rb != nullptr) {
        delete rb;
        rb = nullptr;
    }


    EVP_cleanup();
    if (ssl) {
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
    return 0;
}

int TlsClient::Write(const char *data, int len) {
    int nleft = 0;
    int nsend = 0;
    if (sock) {
        std::unique_lock<std::mutex> lock(lockSend);
        const char *ptr = data;
        nleft = len;
        while (nleft > 0) {
            if ((nsend = SSL_write(ssl, ptr, nleft)) < 0) {
                if (errno == EINTR) {
                    nsend = 0;          /* and call send() again */
                } else {
                    printf("消息data='%s' nsend=%d, 错误代码是%d, 错误信息是'%s'\n", data, nsend, errno,
                           strerror(errno));
                    return (-1);
                }
            } else if (nsend == 0) {
                break;                  /* EOF */
            }
            nleft -= nsend;
            ptr += nsend;
        }
    }

    return nsend;
}
