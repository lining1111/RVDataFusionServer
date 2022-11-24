//
// Created by lining on 11/19/22.
//

#ifndef _PACKETLOSS_HPP
#define _PACKETLOSS_HPP

#include <cstdint>
#include "timeTask.hpp"

using namespace std;
namespace moniter {
    class PacketLoss {
    private:
        mutex mtx;
        uint64_t loss = 0;
        uint64_t sum = 0;
        uint64_t success = 0;
        double ratioLoss = 0.0;
        double ratioSuccess = 0.0;
        Timer *timer;
        int timeval_ms;
        bool isInterval = true;
    public:
        void Success() {
            unique_lock<std::mutex> lck(mtx);
            sum++;
            success++;
            if (sum == numeric_limits<uint64_t>::max()) {
                sum = 0;
                success = 0;
            }
        }

        void Fail() {
            unique_lock<std::mutex> lck(mtx);
            sum++;
            loss++;
            if (sum == numeric_limits<uint64_t>::max()) {
                sum = 0;
                loss = 0;
            }
        }

        double ShowLoss() {
            unique_lock<std::mutex> lck(mtx);
            return ratioLoss;
        }

        double ShowSuccess() {
            unique_lock<std::mutex> lck(mtx);
            return ratioSuccess;
        }

    private:
        void clear() {
            sum = 0;
            loss = 0;
            success = 0;
        }

        static void Cal(void *p) {
            if (p == nullptr) {
                return;
            }
            auto self = reinterpret_cast<PacketLoss *>(p);
            unique_lock<std::mutex> lck(self->mtx);
            if (self->sum != 0) {
                self->ratioLoss = (static_cast<double>(self->loss) / static_cast<double>(self->sum));
                self->ratioSuccess = (static_cast<double>(self->success) / static_cast<double>(self->sum));
                if (self->isInterval) {
                    //清除历史数据
                    self->clear();
                }
            }
        }

    public:
        PacketLoss(int timeval_ms, bool enableInterval = true) {
            this->timeval_ms = timeval_ms;
            timer = new Timer("packetLoss");
            //开启定时任务
            timer->start(timeval_ms, std::bind(Cal, this));
        }

        ~PacketLoss() {
            delete timer;
        }
    };
}


#endif //_PACKETLOSS_HPP
