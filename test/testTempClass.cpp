//
// Created by lining on 11/20/22.
//

#include <vector>
#include <iostream>
#include "Queue.hpp"
#include "data/DataUnit.h"
#include "common/common.h"

using namespace std;
using namespace common;

template<typename T,typename T1>
class Test {
public:
    Queue<T> data;
    Queue<T1> data1;
    int num;

    Test(int cap, int num) {
        data.setMax(cap);
        this->num = num;
    }

    ~Test() {

    }
};


int main() {

    vector<queue<int>> a_v;
    a_v.resize(4);
    a_v.at(1).push(1);

    Test<int,double> test(10, 20);
    cout << "num:" << to_string(test.num) << endl;

}