//
// Created by lining on 2023/5/29.
//

#ifndef VECTOR_H
#define VECTOR_H

#include <vector>
#include <pthread.h>

using namespace std;


template<typename T>
class Vector {
public:
    Vector() {
        // 锁的初始化
        pthread_mutex_init(&mutex, 0);
    }

    Vector(unsigned int max) {
        // 锁的初始化
        pthread_mutex_init(&mutex, 0);
        setMax(max);
    }

    ~Vector() {
        // 锁的释放
        pthread_mutex_destroy(&mutex);
    }

    bool push(T t) {
        pthread_mutex_lock(&mutex);  //加锁
        //先将数据压入
        q.push_back(t);

        //当设定最大值的时候,如果达到最大值,将头部数据删除
        if (isSetMax) {
            if (q.size() > max) {
                q.erase(q.begin());
            }
        }
        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }

    bool getIndex(T &t, int index) {
        if ((q.size() < (index + 1)) || index < 0 || q.empty()) {
            return false;
        }
        pthread_mutex_lock(&mutex);  //加锁
        t = q.at(index);
        pthread_mutex_unlock(&mutex);  //操作完成后解锁
        return true;
    }

    void eraseBegin() {
        if (!q.empty()) {
            pthread_mutex_lock(&mutex);  //加锁
            q.erase(q.begin());
            pthread_mutex_unlock(&mutex);  //操作完成后解锁
        }
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

    void clear() {
        vector<T> q1;
        swap(q, q1);
    }

private:
    int max;
    bool isSetMax = false;
    vector<T> q;
    pthread_mutex_t mutex;

};


#endif //VECTOR_H
