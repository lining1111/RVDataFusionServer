//
// Created by lining on 2023/11/15.
//

#include "ReSendQueue.h"

#include "myTcp/MyTcpClient.hpp"
#include "localBussiness/localBusiness.h"
#include <glog/logging.h>

ReSendQueue::ReSendQueue() {
    mtx = new mutex();
    msgQueue.clear();

    startBusiness();
}

ReSendQueue::ReSendQueue(uint64_t max) : max(max) {
    mtx = new mutex();
    msgQueue.clear();

    startBusiness();
}

ReSendQueue::~ReSendQueue() {
    stopBusiness();
    msgQueue.clear();
    if (mtx != nullptr) {
        delete mtx;
    }
}

void ReSendQueue::startBusiness() {
    LOG(WARNING) << "ReSendQueue startBusiness";
    isRun = true;
    _t = std::thread([this]() {
        LOG(WARNING) << "ReSendQueue start";
        while (this->isRun) {
            sleep(this->interval);
            //遍循待发送队列
            std::unique_lock<std::mutex> lock(*mtx);
            for (int i = 0; i < msgQueue.size(); i++) {
                auto iter = msgQueue.at(i);
                //开始重发
                int result = doSend(iter);
                //判断重复的结果，成功的话，踢出重发队列，失败的话，判断重发条件，不满足重发条件的踢出重发队列
                if (result == 0) {
                    LOG(WARNING) << "重发:踢出重发队列:" << iter.name << "-" << iter.type << ",cmd:"
                                 << iter.pkg.head.cmd
                                 << ",帧内时间:" << iter.timestampFrame << ",插入时间:" << iter.timestampInsert;
                    msgQueue.erase(msgQueue.begin() + i);
                } else {
                    if (judge(iter)) {
                        LOG(WARNING) << "重发:踢出重发队列:" << iter.name << "-" << iter.type << ",cmd:"
                                     << iter.pkg.head.cmd
                                     << ",帧内时间:" << iter.timestampFrame << ",插入时间:" << iter.timestampInsert;
                        msgQueue.erase(msgQueue.begin() + i);
                    }
                }
            }
        }
        LOG(WARNING) << "ReSendQueue end";
    });
    _t.detach();

}

void ReSendQueue::stopBusiness() {
    LOG(WARNING) << "ReSendQueue stopBusiness";
    isRun = false;
}

int ReSendQueue::add(ReSendQueue::Msg msg) {
    std::unique_lock<std::mutex> lock(*mtx);
    if (msgQueue.size() >= max) {
        return -1;
    } else {
        msg.countResend = 0;
        msg.timestampInsert = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        msgQueue.push_back(msg);
    }
    LOG(WARNING) << "重发:加入重发队列:" << msg.name << "-" << msg.type << ",cmd:" << msg.pkg.head.cmd
                 << ",帧内时间:" << msg.timestampFrame << ",插入时间:" << msg.timestampInsert;
    return 0;
}

int ReSendQueue::getQueueLen() {
    std::unique_lock<std::mutex> lock(*mtx);
    int size = msgQueue.size();
    return size;
}

int ReSendQueue::doSend(ReSendQueue::Msg &msg) {
    int ret = -1;
    auto local = LocalBusiness::instance();
    for (auto cli: local->clientList) {
        if (cli.first == msg.name) {
            if (!cli.second->isNeedReconnect) {
                uint64_t timestampStart = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                int ret1 = cli.second->SendBase(msg.pkg);
                uint64_t timestampEnd = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                if (ret1 != 0) {
                    //重发失败
                    LOG(WARNING) << "重发失败:" << msg.name << "," << msg.type << ",msg cmd:" << msg.pkg.head.cmd;
                } else {
                    //重发成功
                    LOG(WARNING) << "重发成功:" << msg.name << "," << msg.type << ",msg cmd:" << msg.pkg.head.cmd;
                    ret = 0;
                }
                //更新信息
                msg.timestampResend = timestampStart;
                msg.countResend++;
            }
            break;
        }
    }
    return ret;

}

bool ReSendQueue::judge(ReSendQueue::Msg msg) {
    bool ret = false;

    //根据类型判断条件
    switch (msg.type) {
        case RESEND_TYPE_NONE: {

        }
            break;
        case RESEND_TYPE_TIME: {
            if (abs((long long) msg.timestampResend - (long long) msg.timestampInsert) >= msg.threshold) {
                ret = true;
            }
        }
            break;
        case RESEND_TYPE_COUNT: {
            if (msg.countResend >= msg.threshold) {
                ret = true;
            }
        }
            break;
        default:

            break;

    }

    return ret;
}
