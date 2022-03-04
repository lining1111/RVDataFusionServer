//
// Created by lining on 3/3/22.
//

#include "GetData.h"
#include <dirent.h>
#include <iostream>
#include <algorithm>

using namespace std;

GetData::GetData(string file) {
    ifstream in(file, ios::in);
    //按行读取文件内容
    string line;
    while (!in.eof()) {
        getline(in, line);
        stringstream input(line);
        string result;
        vector<string> item;
        while (input >> result) {
            item.push_back(result);
        }
        data.push_back(item);
    }
    in.close();
}


GetData::~GetData() {
    data.clear();
}

void GetFiles(string path, vector<string> &files) {
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        cout << path << " is not a directory" << endl;
        return;
    }
    struct dirent *dirent = nullptr;//dirent会存储文件的各种属性
    string dot = ".";
    string dotdot = "..";
    while ((dirent = readdir(dir)) != nullptr) {
        //忽略. ..
        if (string(dirent->d_name) == dot || string(dirent->d_name) == dotdot) {
            continue;
        }
        if (dirent->d_type == DT_REG) {
            string name(dirent->d_name);
            files.push_back(name);
        }
    }
    closedir(dir);

}

static string getCameraTimestamp(string in) {
    //时间戳m_ct1645679190529_rt1645679189565.txt
    int begin = in.find("_ct");
    int end = in.find("_rt");
    if (begin != in.npos && end != in.npos) {
        return in.substr(begin + 3, (end - (begin + 3)));
    }

}

static bool compareTimestamp(string a, string b) {
    //分别获取 a b的 时间戳m_ct1645679190529_rt1645679189565.txt
    string aT = getCameraTimestamp(a);
    string bT = getCameraTimestamp(b);

    uint64_t aV = atoll(aT.c_str());
    uint64_t bV = atoll(bT.c_str());

    return aV < bV;

}

int GetOrderListFileName(string path, vector<string> &vectorFileName) {
    int ret = 0;

    //1.先获取无序的文件名集合
    vector<string> files;
    GetFiles(path, files);

    //2.剔除文件名不合格的文件，m_ct1645679190529_rt1645679189565.txt，同时具有_ct _rt
    for (int i = 0; i < files.size(); i++) {
        auto iter = files.at(i);
        int begin = iter.find("_ct");
        int end = iter.find("_rt");
        if (begin == iter.npos || end == iter.npos) {
            files.erase(files.begin() + i);
            continue;
        }

        string cT;
        uint64_t cV = 0;
        string rT;
        uint64_t rV = 0;
        if (begin != iter.npos) {
            cT = iter.substr(begin + 3, (end - (begin + 3)));
            cV = atoll(cT.c_str());
        }
        if (end != iter.npos) {
            rT = iter.substr(end + 3, ((iter.length() - 4) - (end + 3)));
            rV = atoll(rT.c_str());
        }
        if (cV == 0 || rV == 0) {
            files.erase(files.begin() + i);
        }
    }
    files.shrink_to_fit();

    //3.排列文件名
    sort(files.begin(), files.end(), compareTimestamp);

    //4.将结果拷贝到
    vectorFileName.assign(files.begin(), files.end());

    ret = vectorFileName.size();
    return ret;
}
