//
// Created by lining on 2/21/22.
//函数命令行设置参数
//

#ifndef _PARSEFLAG_H
#define _PARSEFLAG_H
#include <map>

using namespace std;
class ParseFlag {
public:
    map<string,string> useAgeAll;//可以设置的
    map<string,string> useAgeSet;//已经设置的
public:
    ParseFlag(map<string,string> useAge);
    ~ParseFlag();

public:
    void Parse(int argc, char** argv);
};


#endif //_PARSEFLAG_H
