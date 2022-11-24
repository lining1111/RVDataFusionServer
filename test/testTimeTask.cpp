#include <cstdlib>
#include <csignal>
#include <iostream>
#include <sys/time.h>
#include <map>

#include "timeTask.hpp"
//
// Created by lining on 11/17/22.
//
using namespace std;

class A {
public:
    static void fun() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        cout << "tv:" << to_string(tv.tv_sec) << ":" << to_string(tv.tv_usec) << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
};

void test() {
    A a;
    Timer *timer = new Timer("timer1");
    std::cout << "--- start period timer ----" << std::endl;
    timer->start(100, std::bind(a.fun));

//    timer.stop();
//    std::cout << "--- stop period timer ----" << std::endl;
    std::map<string, Timer> timers;
    timers.insert(make_pair("test", *timer));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "--- stop period timer ----" << std::endl;
    auto iter = timers.find("test");
    iter->second.stop();
    iter = timers.erase(iter);
    auto iter1 = timers.find("test");
    if (iter != timers.end()) {
        std::cout << "删除失败" << endl;
    }
}


int main() {
    test();
}