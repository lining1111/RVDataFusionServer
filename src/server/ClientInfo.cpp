//
// Created by lining on 2/15/22.
//

#include <cstring>
#include <sys/time.h>
#include <unistd.h>
#include "server/ClientInfo.h"
#include "log/Log.h"

using namespace log;

ClientInfo::ClientInfo(struct sockaddr_in clientAddr, int client_sock, long long int rbCapacity) {
    this->msgid = 0;
    this->clientAddr = clientAddr;
    this->sock = client_sock;
    this->clientType = 0;
    bzero(this->extraData, sizeof(this->extraData) / sizeof(this->extraData[0]));
    gettimeofday(&this->receive_time, nullptr);
    this->rb = RingBuffer_New(rbCapacity);
    this->status = Start;
}

ClientInfo::~ClientInfo() {

    if (sock > 0) {
        close(sock);
    }

    if (isLive) {
        isLive = false;
    }

    RingBuffer_Delete(this->rb);
}

int ClientInfo::SetQueuePkg(Pkg &pkg) {

    //最多缓存600
    if (this->queuePkg.size() >= maxQueuePkg) {
        return -1;
    }


    pthread_mutex_lock(&this->lockPkg);

    //存入队列
    queuePkg.push(pkg);

    pthread_cond_broadcast(&this->condPkg);
    pthread_mutex_unlock(&this->lockPkg);

    return 0;
}

int ClientInfo::GetQueuePkg(Pkg &pkg) {
    if (this->queuePkg.empty()) {
        return -1;
    }

    pthread_mutex_lock(&this->lockPkg);

    if (this->queuePkg.empty()) {
        pthread_cond_wait(&this->condPkg, &this->lockPkg);
    }

    //取出队列头
    pkg = this->queuePkg.front();
    this->queuePkg.pop();

    pthread_mutex_unlock(&this->lockPkg);

    return 0;
}

int ClientInfo::SetQueueWatchData(WatchData &watchData) {
    //最多缓存600
    if (this->queueWatchData.size() >= maxQueueWatchData) {
        return -1;
    }


    pthread_mutex_lock(&this->lockWatchData);

    //存入队列
    queueWatchData.push(watchData);

    pthread_cond_broadcast(&this->condWatchData);
    pthread_mutex_unlock(&this->lockWatchData);

    return 0;
}

int ClientInfo::GetQueueWatchData(WatchData &watchData) {
    if (this->queueWatchData.empty()) {
        return -1;
    }

    pthread_mutex_lock(&this->lockWatchData);

    if (this->queueWatchData.empty()) {
        pthread_cond_wait(&this->condWatchData, &this->lockWatchData);
    }

    //取出队列头
    watchData = this->queueWatchData.front();
    this->queueWatchData.pop();

    pthread_mutex_unlock(&this->lockWatchData);

    return 0;
}

int ClientInfo::ProcessRecv() {
    isLive = true;

    //dump
    threadDump = thread(ThreadDump, this);
    string name = "ClientDump" + to_string(this->sock);
    pthread_setname_np(threadDump.native_handle(), name.data());
    threadDump.detach();
    //getPkg
    threadGetPkg = thread(ThreadGetPkg, this);
    string name1 = "ClientGetPkg" + to_string(this->sock);
    pthread_setname_np(threadGetPkg.native_handle(), name1.data());
    threadGetPkg.detach();
    //getPkgContent
    threadGetPkgContent = thread(ThreadGetPkgContent, this);
    string name2 = "ClientGetPkgContent" + to_string(this->sock);
    pthread_setname_np(threadGetPkgContent.native_handle(), name2.data());
    threadGetPkgContent.detach();

    return 0;
}

void ClientInfo::ThreadDump(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("%s run", __FUNCTION__);
    while (client->isLive) {
        uint8_t buffer[1024 * 128];
        int len = 0;
        len = recv(client->sock, buffer, ARRAY_SIZE(buffer), 0);
        if (len == -1) {
            Error("recv sock %d err:%s", client->sock, strerror(errno));
            //向服务端抛出应该关闭
            client->needRelease = true;
        } else if (len > 0) {
            //将数据存入缓存
            RingBuffer_Write(client->rb, buffer, len);
        }
    }

    Info("%s exit", __FUNCTION__);

}

void ClientInfo::ThreadGetPkg(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("%s run", __FUNCTION__);
    while (client->isLive) {

        PkgHead pkgHead;//分包头
        int bodyLen = 0;//获取分包头后，得到的包长度
        uint8_t *pkgBuffer = nullptr;//分包缓冲
        int index = 0;//分包缓冲的索引

        usleep(10);

        if (client->rb->available_for_read == 0) {
            //无数据可读
            continue;
        }

        //分包状态机
        switch (client->status) {
            case Start: {
                //开始,寻找包头的开始标志
                if (client->rb->available_for_read > MEMBER_SIZE(PkgHead, tag)) {
                    char start = '\0';
                    RingBuffer_Read(client->rb, &start, 1);
                    if (start == '$') {
                        //找到开始标志
                        client->status = GetHeadStart;
                    }
                }
            }
                break;
            case GetHeadStart: {
                //开始寻找头
                if (client->rb->available_for_read > (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag))) {
                    //获取完整的包头
                    pkgHead.tag = '$';
                    RingBuffer_Read(client->rb, &pkgHead.version, (sizeof(PkgHead) - MEMBER_SIZE(PkgHead, tag)));
                    //buffer申请内存
                    pkgBuffer = (uint8_t *) calloc(1, pkgHead.len);
                    index = 0;
                    memcpy(pkgBuffer, &pkgHead, sizeof(pkgHead));
                    index += sizeof(pkgHead);
                    client->status = GetBody;
                    bodyLen = pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                }
            }
                break;
            case GetHead: {
                if (bodyLen <= 0) {
                    bodyLen = pkgHead.len - sizeof(PkgHead) - sizeof(PkgCRC);
                }
                if (client->rb->available_for_read > bodyLen) {
                    //获取正文
                    RingBuffer_Read(client->rb, pkgBuffer + index, bodyLen);
                    index += bodyLen;
                    client->status = GetBody;
                }
            }
                break;
            case GetBody: {
                if (client->rb->available_for_read > sizeof(PkgCRC)) {
                    //获取CRC
                    RingBuffer_Read(client->rb, pkgBuffer + index, sizeof(PkgCRC));
                    index += sizeof(PkgCRC);
                    client->status = GetCRC;
                }
            }
                break;
            case GetCRC: {
                //获取CRC 就获取全部的分包内容，转换为结构体
                Pkg pkg;

                Unpack(pkgBuffer, pkgHead.len, pkg);

                //存入分包队列
                int set = client->SetQueuePkg(pkg);
                if (set != 0) {
                    Info("分包队列已满，丢弃此包:%s-%s",
                         pkg.body.methodName.name.c_str(),
                         pkg.body.methodParam.param.c_str());
                }

                int bodyLen = 0;//获取分包头后，得到的包长度
                if (pkgBuffer) {
                    free(pkgBuffer);
                    pkgBuffer = nullptr;//分包缓冲
                }
                int index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
            default: {
                //意外状态错乱后，回到最初
                int bodyLen = 0;//获取分包头后，得到的包长度
                if (pkgBuffer) {
                    free(pkgBuffer);
                    pkgBuffer = nullptr;//分包缓冲
                }
                int index = 0;//分包缓冲的索引
                client->status = Start;
            }
                break;
        }

    }

    Info("%s exit", __FUNCTION__);

}

void ClientInfo::ThreadGetPkgContent(void *pClientInfo) {
    if (pClientInfo == nullptr) {
        return;
    }

    auto client = (ClientInfo *) pClientInfo;

    Info("%s run", __FUNCTION__);
    while (client->isLive) {
        usleep(10);
        if (client->queuePkg.empty()) {
            //分别队列为空
            continue;
        }
        Pkg pkg;
        int get = client->GetQueuePkg(pkg);
        if (get != 0) {
            Info("获取分包失败");
        } else {
            //解析分包，按照方法名不同，存入不同的队列
            if (string(pkg.body.methodName.name) == Method(WatchData)) {
                //"WatchData"
                WatchData watchData;
                JsonUnmarshalWatchData(string(pkg.body.methodParam.param), watchData);
                //存入队列
                int set = client->SetQueueWatchData(watchData);
                if (set != 0) {
                    Info("WatchData队列已满,丢弃消息:%s-%s", pkg.body.methodName.name.c_str(),
                         pkg.body.methodParam.param.c_str());
                }
            } else {
                //最后没有对应的方法名
                Info("最后没有对应的方法名:%s,内容:%s", pkg.body.methodName.name.c_str(), pkg.body.methodParam.param.c_str());
            }
        }


    }

    Info("%s exit", __FUNCTION__);

}
