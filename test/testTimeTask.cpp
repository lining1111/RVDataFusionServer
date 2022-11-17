#include <cstdlib>
#include <csignal>
#include <iostream>
#include <sys/time.h>

#include "timeTask.hpp"
//
// Created by lining on 11/17/22.
//
using namespace std;

void fun() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    cout << "tv:" << to_string(tv.tv_sec) << ":" << to_string(tv.tv_usec) << endl;
}


int main() {
    Timer timer;
    std::cout << "--- start period timer ----" << std::endl;
    timer.start(1000, std::bind(fun));
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    timer.stop();
    std::cout << "--- stop period timer ----" << std::endl;
}