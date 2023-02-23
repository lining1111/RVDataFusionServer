//
// Created by lining on 9/13/22.
//

#ifndef _TLSCLIENT_H
#define _TLSCLIENT_H

#include <string>
#include "ringbuffer/RingBuffer.h"
#include <future>
#include <atomic>
#include <queue>
#include <openssl/ossl_typ.h>

using namespace std;

class TlsClient {
private:
//    string serverName;
    string serverIp;
    int serverPort;
    int sock;
    pthread_mutex_t lockSend = PTHREAD_MUTEX_INITIALIZER;
    string serverCAPath;
//    string CertificateFilePath;
//    string privateKeyPath;
    SSL_CTX *ctx;
    SSL *ssl;
public:
    atomic_bool isLive;

    RingBuffer *rb = nullptr;//接收数据缓存环形buffer
    //客户端处理线程
    bool isLocalThreadRun = false;
    std::shared_future<int> futureDump;//接收数据并存入环形buffer
public:
    TlsClient(string ip, int port, string caPath);

    virtual ~TlsClient();

    bool isAsynRecieve = true;

private:

    int connectServer();

    void printErrorString(unsigned long err, const char *const label);

    static int verifyCB(int preVerify, X509_STORE_CTX *x509_ctx);

    void showCerts(SSL *ssl);

    /**
    * 接收数据缓冲线程
    * @param pClient
    */
    static int ThreadDump(void *pClient);

public:
    int Open();

    int Run();

    int Close();

    int Write(const char *data, int len);

};


#endif //_TLSCLIENT_H
