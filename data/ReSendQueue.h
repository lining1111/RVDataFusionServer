//
// Created by lining on 2023/11/15.
//

#ifndef RESENDQUEUE_H
#define RESENDQUEUE_H

#include <vector>
#include <mutex>
#include <thread>

using namespace std;

#include "common/common.h"

/**
 *  重发队列
 */

class ReSendQueue {
public:

    //重发类型，按时间和按次数
    typedef enum {
        RESEND_TYPE_NONE = 0,//无限重发
        RESEND_TYPE_TIME = 1,//按时间重发
        RESEND_TYPE_COUNT = 2,//按次数重发
    } Type;

    typedef struct {
        common::Pkg pkg;
        uint64_t timestampFrame = 0;//帧时间
        uint64_t timestampInsert = 0;//插入时间
        uint64_t timestampResend = 0;//执行重发时间
        uint64_t countResend = 0;//重发次数
        uint64_t threshold = 0;//阈值:与重发类型配合
        Type type;
        string name;//重发的客户端名称，与Business中的客户端数组配合使用
    } Msg;
private:
    bool isRun = false;
    std::thread _t;
    mutex *mtx = nullptr;
    vector<Msg> msgQueue;
    uint64_t max = 1024 * 64;//最大容量
    uint64_t interval = 1;//业务间隔，秒
public:
    ReSendQueue();

    ReSendQueue(uint64_t max);

    ~ReSendQueue();

    void startBusiness();

    void stopBusiness();

    int add(Msg msg);

    int getQueueLen();

    int doSend(Msg &msg);

    /**
     * 判定是否应该踢出队列
     * @param msg
     * @return
     */
    bool judge(Msg msg);
};


#endif //RESENDQUEUE_H
