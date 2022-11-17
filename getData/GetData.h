//
// Created by lining on 3/3/22.
// 从txt文档中读取雷达和视频的融合数据以行为分界线，行内以空格分割
//

#ifndef _GETDATA_H
#define _GETDATA_H

#include <cstdio>
#include <vector>
#include <csignal>
#include <memory>
#include <fstream>
#include <sstream>
#include "common/common.h"

using namespace common;

using namespace std;

class GetData {
public:
    //文件集合的路径
    string path;
    //排序好的文件列表，按摄像头时间顺序
    vector<string> files;
    //临时数据
    //解析出来的数据
    vector<vector<string >> data;//二维数组，行(文本有多少行) 列(文本每行有多少列)
    //具体的数据含义分解
    vector<ObjTarget> obj;//有多少单路RV数据

public:
    GetData(string path);

    ~GetData();

private:
    static string getCameraTimestamp(string in);

    static bool compareTimestamp(string a, string b);

    /**
    * 获取目录下文件名集合，未排序
    * @param path in目录
    * @param files out 文件名集合
    */
    void GetFiles(string path, vector<string> &files);

public:
    /**
     * 获取目录下按指定规则排列好的文件名集合
     * 这里的规则是按照摄像机采集时间排列
     * @param path in目录名称
     * @param vectorFileName out 排列好的集合
     * @return 集合个数
     */
    int GetOrderListFileName(string path);

    /**
     * 从一个文件中获取数据的二维数据
     * @param file in 文件名
     * @return 一共多少行
     */
    int GetDataFromOneFile(string file);

    /**
     * 从数据的二维数组中获取结构体
     * @param data
     * @return
     */
    int GetObjFromData(vector<vector<string>> data);

};


#endif //_GETDATA_H
