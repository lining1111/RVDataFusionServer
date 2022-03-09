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
    enum ArgOption{
        SinglePole,//”-“
        DoublePole,//”--“
    };

public:
    string option = "-";
    map<string,string> useAgeAll;//可以设置的
    map<string,string> useAgeSet;//已经设置的
public:
    ParseFlag(map<string, string> useAge, ArgOption argOption);
    ~ParseFlag();

public:
    /**
     * 传入 cmd参数
     * @param argc
     * @param argv
     */
    void Parse(int argc, char** argv);

    /**
     * 显示帮助信息
     */
    void ShowHelp();
};


#endif //_PARSEFLAG_H
