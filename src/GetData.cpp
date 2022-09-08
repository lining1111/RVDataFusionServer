//
// Created by lining on 3/3/22.
//

#include "GetData.h"
#include <dirent.h>
#include <iostream>
#include <algorithm>

using namespace std;

GetData::GetData(string path) {
    this->path = path;
    this->files.clear();
    this->data.clear();
    this->obj.clear();
}


GetData::~GetData() {
    data.clear();
    obj.clear();
}

void GetData::GetFiles(string path, vector<string> &files) {
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

string GetData::getCameraTimestamp(string in) {
    //时间戳m_ct1645679190529_rt1645679189565.txt
    int begin = in.find("_ct");
    int end = in.find("_rt");
    if (begin != in.npos && end != in.npos) {
        return in.substr(begin + 3, (end - (begin + 3)));
    }

}

bool GetData::compareTimestamp(string a, string b) {
    //分别获取 a b的 时间戳m_ct1645679190529_rt1645679189565.txt
    string aT = getCameraTimestamp(a);
    string bT = getCameraTimestamp(b);

    uint64_t aV = atoll(aT.c_str());
    uint64_t bV = atoll(bT.c_str());

    return aV < bV;

}

int GetData::GetOrderListFileName(string path) {
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
    this->files.assign(files.begin(), files.end());

    ret = this->files.size();
    return ret;
}

int GetData::GetDataFromOneFile(string file) {
    ifstream in(file, ios::in);
    //按行读取文件内容
    string line;
    int count = 0;
    this->data.clear();
    while (!in.eof()) {
        if (getline(in, line)) {
            stringstream input(line);
            string result;
            vector<string> item;
            while (input >> result) {
                item.push_back(result);
            }
            data.push_back(item);
            count++;
        }
    }
    in.close();
    return count;
}

int GetData::GetObjFromData(vector<vector<string>> data) {
    //数据每行和结构体的对应关系
    //objID objCameraID left top right bottom locationX locationY directionAngle distance speed
    int count = 0;
    this->obj.clear();
    for (int i = 0; i < data.size(); i++) {
        auto iter = data.at(i);
        ObjTarget item;
        int index = 0;
        //objID
        if ((index + 1) <= iter.size()) {
            item.objID = atoi(iter.at(index).c_str());
            index++;
        }
        //objCameraID
        if ((index + 1) <= iter.size()) {
            item.objCameraID = atoi(iter.at(index).c_str());
            index++;
        }
        //left
        if ((index + 1) <= iter.size()) {
            item.left = atoi(iter.at(index).c_str());
            index++;
        }
        //top
        if ((index + 1) <= iter.size()) {
            item.top = atoi(iter.at(index).c_str());
            index++;
        }
        //right
        if ((index + 1) <= iter.size()) {
            item.right = atoi(iter.at(index).c_str());
            index++;
        }
        //bottom
        if ((index + 1) <= iter.size()) {
            item.bottom = atoi(iter.at(index).c_str());
            index++;
        }
        //locationX
        if ((index + 1) <= iter.size()) {
            item.locationX = atof(iter.at(index).c_str());
            index++;
        }
        //locationY
        if ((index + 1) <= iter.size()) {
            item.locationY = atof(iter.at(index).c_str());
            index++;
        }
        //directionAngle
        if ((index + 1) <= iter.size()) {
            item.directionAngle = atof(iter.at(index).c_str());
            index++;
        }
        //distance
        if ((index + 1) <= iter.size()) {
            item.distance = iter.at(index);
            index++;
        }
        //speed
//        if ((index + 1) <= iter.size()) {
//            item.speed = iter.at(index);
//            index++;
//        }

        this->obj.push_back(item);
        count++;
    }

    return count;
}
