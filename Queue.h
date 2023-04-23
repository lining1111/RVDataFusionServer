//
// Created by lining on 10/1/22.
//

#ifndef _QUEUE_H
#define _QUEUE_H


#include <queue>
#include <sys/time.h>
#include <pthread.h>

using namespace std;


template<typename T>
class Queue {
public:
    Queue() {
        // 锁的初始化
        pthread_mutex_init(&mutex, 0);
        // 线程条件变量的初始化
        pthread_cond_init(&cond, 0);
    }

    Queue(unsigned int max) {
        // 锁的初始化
        pthread_mutex_init(&mutex, 0);
        // 线程条件变量的初始化
        pthread_cond_init(&cond, 0);
        setMax(max);
    }

    ~Queue() {
        // 锁的释放
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    bool push(T t) {
        if (isSetMax) {
            if (q.size() >= max) {
                return false;
            }
        }
        pthread_mutex_lock(&mutex);  //加锁
        q.push(t);
        // 通知变化 notify
        // 由系统唤醒一个线程
        pthread_cond_signal(&cond);
        // 通知所有的线程
//        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }

    bool pop(T &t) {
        pthread_mutex_lock(&mutex);  //加锁
        // queue为空是一直等待，直到下一次push进新的数据  java中是wait和notify
        if (q.empty()) {
            // 挂起状态，释放锁
            struct timespec ts;
//            struct timeval now;
//            long timeout_ms = 3000;//wait time 100ms
//            gettimeofday(&now, nullptr);
//            long nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000 * 1000;//ns
            ts.tv_sec = ts.tv_sec+1;//
            pthread_cond_timedwait(&cond, &mutex, &ts);
        }
        if (q.empty()) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        // 被唤醒以后
        t = q.front();
        q.pop();

        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }

    bool front(T &t) {
        pthread_mutex_lock(&mutex);  //加锁
        // queue为空是一直等待，直到下一次push进新的数据  java中是wait和notify
        if (q.empty()) {
            // 挂起状态，释放锁
            struct timespec ts;
//            struct timeval now;
//            long timeout_ms = 3000;//wait time 100ms
//            gettimeofday(&now, nullptr);
//            long nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000 * 1000;//ns
            ts.tv_sec = ts.tv_sec+1;//
            pthread_cond_timedwait(&cond, &mutex, &ts);
        }
        if (q.empty()) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        // 被唤醒以后
        t = q.front();
        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }

    bool back(T &t) {
        pthread_mutex_lock(&mutex);  //加锁
        // queue为空是一直等待，直到下一次push进新的数据  java中是wait和notify
        if (q.empty()) {
            //挂起状态，释放锁
            struct timespec ts;
//            struct timeval now;
//            long timeout_ms = 3000;//wait time 100ms
//            gettimeofday(&now, nullptr);
//            long nsec = now.tv_usec * 1000 + (timeout_ms % 1000) * 1000 * 1000;//ns
            ts.tv_sec = ts.tv_sec+1;//
            pthread_cond_timedwait(&cond, &mutex, &ts);
        }
        if (q.empty()) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        // 被唤醒以后
        t = q.back();
        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }


    void setMax(int value) {
        max = value;
        isSetMax = true;
    }

    int size() {
        return q.size();
    }

    bool empty() {
        return q.empty();
    }

private:
    // 如何保证对这个队列的操作是线程安全的？引入互斥锁

    queue<T> q;

    int max;
    bool isSetMax = false;
    pthread_mutex_t mutex;

//    // 创建条件变量
    pthread_cond_t cond;

};

#endif //_QUEUE_H
