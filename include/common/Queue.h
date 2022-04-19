//
// Created by lining on 2/18/22.
//自定义带锁队列 可以定义队列的最大容量，超出容量后，不能再存入
//

#ifndef _QUEUE_H
#define _QUEUE_H

#include <queue>

using namespace std;

template<typename T>
class Queue {

private:
    queue<T> q;
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

public:
    /**
     * 存入队列
     * @param t
     * @return 0:success -1:fail
     */
    int Push(T &t);

    /**
     * 获取队列的头部
     * @return 队列元素
     */
    T PopFront();

    /**
     * 获取队列的尾部
     * @return 尾部元素
     */
    T PopBack();

    /**
     * 获取队列的元素个数
     * @return
     */
    int Size();

};


template<typename T>
int Queue<T>::Push(T &t) {

    pthread_mutex_lock(&lock);
    //存入队列
    q.push(t);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    return 0;
}

template<typename T>
T Queue<T>::PopFront() {
    T t;

    pthread_mutex_lock(&lock);
    while (q.empty()) {
        pthread_cond_wait(&cond, &lock);
    }
    //取出队列头
    t = this->q.front();
    this->q.pop();
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    return t;
}

template<typename T>
T Queue<T>::PopBack() {
    T t;

    pthread_mutex_lock(&lock);
    while (q.empty()) {
        pthread_cond_wait(&cond, &lock);
    }
    //取出队列头
    t = this->q.back();
    this->q.pop();
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);

    return t;
}

template<typename T>
int Queue<T>::Size() {
    return q.size();
}


#endif //_QUEUE_H
