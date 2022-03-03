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

using namespace std;

class GetData {
public:
    //解析出来的数据
    vector<vector<string >> data;

public:
    GetData(string file);

    ~GetData();

};

/**
 * 获取目录下文件名集合，未排序
 * @param path in目录
 * @param files out 文件名集合
 */
void GetFiles(string path, vector<string> &files);

/**
 * 获取目录下按指定规则排列好的文件名集合
 * 这里的规则是按照摄像机采集时间排列
 * @param path in目录名称
 * @param vectorFileName out 排列好的集合
 * @return 集合个数
 */
int GetOrderListFileName(string path, vector<string> &vectorFileName);

#endif //_GETDATA_H
