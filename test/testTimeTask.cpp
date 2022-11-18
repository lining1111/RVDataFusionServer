#include <cstdlib>
#include <csignal>
#include <iostream>
#include <sys/time.h>

#include "timeTask.hpp"
//
// Created by lining on 11/17/22.
//
using namespace std;

class A{
public:
    static void fun() {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        cout << "tv:" << to_string(tv.tv_sec) << ":" << to_string(tv.tv_usec) << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (50));
    }
};



int main() {
    A a;
    Timer timer;
    std::cout << "--- start period timer ----" << std::endl;
    timer.start(100, std::bind(a.fun));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    timer.stop();
    std::cout << "--- stop period timer ----" << std::endl;
}