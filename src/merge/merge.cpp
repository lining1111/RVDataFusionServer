//
// Created by lining on 2/18/22.
//

#include "merge/merge.h"

#include <math.h>
#include <vector>
#include <algorithm>
#include <cstring>

#include "log/Log.h"

using namespace z_log;

#define PI 3.1415926
using namespace std;

const int INF = 0x7FFFFFFF;
const int Nmax = 200;


//升序用比较函数
bool target_loc_compare(const OBJECT_INFO_T &a, const OBJECT_INFO_T &b) {
    return a.locationX < b.locationX;
}

bool target_loc_compare_new(const OBJECT_INFO_NEW &a, const OBJECT_INFO_NEW &b) {
    return a.locationX < b.locationX;
}

//初始化最后融合的数据类型
void initvalueNew(OBJECT_INFO_NEW *data, double angle_value) {
    data->objID1 = -INF;
    data->objID2 = -INF;
    data->objID3 = -INF;
    data->objID4 = -INF;
    data->showID = -INF;
    data->cameraID1 = -INF;
    data->cameraID2 = -INF;
    data->cameraID3 = -INF;
    data->cameraID4 = -INF;
    data->objType = -INF;
    memset(data->plate_number, 0x0, 15);
    memset(data->plate_color, 0x0, 7);
    data->left = -INF;
    data->top = -INF;
    data->right = -INF;
    data->bottom = -INF;
    data->locationX = -INF;
    data->locationY = -INF;
    memset(data->distance, 0x0, 10);
    data->speed = -INF;
    data->directionAngle = angle_value;
    data->longitude = -INF;//经度
    data->latitude = -INF;//纬度
    data->flag_new = 1;//默认是新增的id
}


//目标赋值
void get_value(OBJECT_INFO_NEW *obj_info, OBJECT_INFO_T *dataT, OBJECT_INFO_NEW *dataNew) {
    //只能有一个data;另一个data不存在时，输入初始化X=-INF
    if (dataT->locationX != -INF) {
        obj_info->objType = dataT->objType;
        strcpy(obj_info->plate_color, dataT->plate_color);
        strcpy(obj_info->plate_number, dataT->plate_number);
        obj_info->left = dataT->left;
        obj_info->top = dataT->top;
        obj_info->right = dataT->right;
        obj_info->bottom = dataT->bottom;
        obj_info->locationX = dataT->locationX;
        obj_info->locationY = dataT->locationY;
        strcpy(obj_info->distance, dataT->distance);
        obj_info->speed = sqrt(dataT->speedX * dataT->speedX + dataT->speedY * dataT->speedY);
        obj_info->directionAngle = dataT->directionAngle;
        obj_info->longitude = dataT->longitude;
        obj_info->latitude = dataT->latitude;

    }
    if (dataNew->locationX != -INF) {
        obj_info->objType = dataNew->objType;
        strcpy(obj_info->plate_color, dataNew->plate_color);
        strcpy(obj_info->plate_number, dataNew->plate_number);
        obj_info->left = dataNew->left;
        obj_info->top = dataNew->top;
        obj_info->right = dataNew->right;
        obj_info->bottom = dataNew->bottom;
        obj_info->locationX = dataNew->locationX;
        obj_info->locationY = dataNew->locationY;
        strcpy(obj_info->distance, dataNew->distance);
        obj_info->speed = dataNew->speed;
        obj_info->directionAngle = dataNew->directionAngle;
        obj_info->longitude = dataNew->longitude;
        obj_info->latitude = dataNew->latitude;
    }
}


//id搜索
int search_id(OBJECT_INFO_NEW *data_one, vector<OBJECT_INFO_NEW> &data_before) {
    int temp[4], temp_data, temp_databefore, temp_n[4], t, N = data_before.size(), j;
    temp_n[0] = -INF;
    temp_n[1] = -INF;
    temp_n[2] = -INF;
    temp_n[3] = -INF;
    temp[0] = data_one->objID1;
    temp[1] = data_one->objID2;
    temp[2] = data_one->objID3;
    temp[3] = data_one->objID4;
    for (t = 0; t < 4; t++)
        if (temp[t] != -INF) {
            temp_n[t] = 1;
        }
    for (t = 0; t < 4; t++) {
        switch (t) {
            case 0:
                if (temp_n[t] == 1) {
                    temp_data = data_one->objID1;
                    for (j = 0; j < N; j++) {
                        temp_databefore = data_before[j].objID1;
                        if (temp_databefore == temp_data)
                            return j;
                    }
                }
                break;
            case 1:
                if (temp_n[t] == 1) {
                    temp_data = data_one->objID2;
                    for (j = 0; j < N; j++) {
                        temp_databefore = data_before[j].objID2;
                        if (temp_databefore == temp_data)
                            return j;
                    }
                }
                break;
            case 2:
                if (temp_n[t] == 1) {
                    temp_data = data_one->objID3;
                    for (j = 0; j < N; j++) {
                        temp_databefore = data_before[j].objID3;
                        if (temp_databefore == temp_data)
                            return j;
                    }
                }
                break;
            case 3:
                if (temp_n[t] == 1) {
                    temp_data = data_one->objID4;
                    for (j = 0; j < N; j++) {
                        temp_databefore = data_before[j].objID4;
                        if (temp_databefore == temp_data)
                            return j;
                    }
                }
                break;
        }
    }
    return -INF;
}


//对向路口融合
void merge_opposite(vector<OBJECT_INFO_T> &data_one, vector<OBJECT_INFO_T> &data_three, double Gatex, double Gatey,
                    int n_road, vector<OBJECT_INFO_NEW> &data_out, double angle_value) {
    //n_road：若是1、3路口融合，=1；若是2、3路口融合，=2
    int MaxM = data_one.size();
    int MaxN = data_three.size();
    int i = 0, j = 0, index = -INF, id_before, temp_n, delete_n;
    double DeltaX, DeltaY, deltaX, deltaY, min_value, temp, gatex, gatey;
    OBJECT_INFO_NEW obj_info, obj_info_NEW;
    memset(&obj_info_NEW, 0x0, sizeof(OBJECT_INFO_NEW));
    obj_info_NEW.locationX = -INF;

    if (n_road == 1) {
        gatex = Gatey;
        gatey = Gatex;
    } else {
        gatex = Gatex;
        gatey = Gatey;
    }


    std::sort(data_one.begin(), data_one.end(), target_loc_compare);
    std::sort(data_three.begin(), data_three.end(), target_loc_compare);
    for (i = 0; i < MaxM; i++) {
        delete_n = -INF;
        min_value = INF;
        MaxN = data_three.size();
        for (j = 0; j < MaxN; j++) {
            if ((abs(data_one[i].locationX - data_three[j].locationX) <= gatex) &&
                (abs(data_one[i].locationY - data_three[j].locationY) <= gatey)) {
                temp = sqrt((data_one[i].locationX - data_three[j].locationX) *
                            (data_one[i].locationX - data_three[j].locationX) +
                            (data_one[i].locationY - data_three[j].locationY) *
                            (data_one[i].locationY - data_three[j].locationY));
                if (temp < min_value) {
                    min_value = temp;
                    delete_n = j;
                }
            }
        }
        if (min_value != INF) {
            //初始化目标类型
            initvalueNew(&obj_info, angle_value);
            if (n_road == 1) {
                obj_info.objID1 = data_one[i].objID;
                obj_info.objID3 = data_three[delete_n].objID;
                obj_info.cameraID1 = data_one[i].cameraID;
                obj_info.cameraID3 = data_three[delete_n].cameraID;
            } else {
                obj_info.objID2 = data_one[i].objID;
                obj_info.objID4 = data_three[delete_n].objID;
                obj_info.cameraID2 = data_one[i].cameraID;
                obj_info.cameraID4 = data_three[delete_n].cameraID;
            }
            get_value(&obj_info, &data_one[i], &obj_info_NEW);
            data_out.push_back(obj_info);
            //删除匹配成功的成员
            data_three.erase(data_three.begin() + delete_n);
        } else {
            //没有点匹配，保留下data1的点
            //初始化目标类型
            initvalueNew(&obj_info, angle_value);
            if (n_road == 1) {
                obj_info.objID1 = data_one[i].objID;
                obj_info.cameraID1 = data_one[i].cameraID;
            } else {
                obj_info.objID2 = data_one[i].objID;
                obj_info.cameraID2 = data_one[i].cameraID;

            }
            get_value(&obj_info, &data_one[i], &obj_info_NEW);
            data_out.push_back(obj_info);
        }
    }
    //保留下data3剩余的点
    temp_n = data_three.size();
    for (i = 0; i < temp_n; i++) {
        //初始化目标类型
        initvalueNew(&obj_info, angle_value);
        if (n_road == 1) {
            obj_info.objID3 = data_three[i].objID;
            obj_info.cameraID3 = data_three[i].cameraID;
        } else {
            obj_info.objID4 = data_three[i].objID;
            obj_info.cameraID4 = data_three[i].cameraID;
        }
        get_value(&obj_info, &data_three[i], &obj_info_NEW);
        data_out.push_back(obj_info);
    }
}


//最后一次融合
void merge_last(vector<OBJECT_INFO_NEW> &data_one, vector<OBJECT_INFO_NEW> &data_three, double gatex, double gatey,
                vector<OBJECT_INFO_NEW> &data_out, double angle_value) {
    int MaxM = data_one.size();
    int MaxN = data_three.size();
    int i = 0, j = 0, t = 0, index, temp_n, delete_n;
    double DeltaX, DeltaY, deltaX, deltaY, min_value, temp;
    OBJECT_INFO_NEW obj_info;
    OBJECT_INFO_T obj_info_T;
    memset(&obj_info_T, 0x0, sizeof(OBJECT_INFO_T));
    obj_info_T.locationX = -INF;

    std::sort(data_one.begin(), data_one.end(), target_loc_compare_new);
    std::sort(data_three.begin(), data_three.end(), target_loc_compare_new);
    for (i = 0; i < MaxM; i++) {
        delete_n = -INF;
        min_value = INF;
        MaxN = data_three.size();
        for (j = 0; j < MaxN; j++) {
            if ((abs(data_one[i].locationX - data_three[j].locationX) <= gatex) &&
                (abs(data_one[i].locationY - data_three[j].locationY) <= gatex)) {
                temp = sqrt((data_one[i].locationX - data_three[j].locationX) *
                            (data_one[i].locationX - data_three[j].locationX) +
                            (data_one[i].locationY - data_three[j].locationY) *
                            (data_one[i].locationY - data_three[j].locationY));
                if (temp < min_value) {
                    min_value = temp;
                    delete_n = j;
                }
            }
        }
        if (min_value != INF) {
            //初始化目标类型
            initvalueNew(&obj_info, angle_value);
            obj_info.objID1 = data_one[i].objID1;
            obj_info.objID3 = data_one[i].objID3;
            obj_info.objID2 = data_three[delete_n].objID2;
            obj_info.objID4 = data_three[delete_n].objID4;
            obj_info.cameraID1 = data_one[i].cameraID1;
            obj_info.cameraID3 = data_one[i].cameraID3;
            obj_info.cameraID2 = data_three[delete_n].cameraID2;
            obj_info.cameraID4 = data_three[delete_n].cameraID4;
            get_value(&obj_info, &obj_info_T, &data_one[i]);
            data_out.push_back(obj_info);
            //删除匹配成功的成员
            data_three.erase(data_three.begin() + delete_n);
        } else {
            //没有点匹配，保留下data1的点
            //初始化目标类型
            initvalueNew(&obj_info, angle_value);
            obj_info.objID1 = data_one[i].objID1;
            obj_info.objID3 = data_one[i].objID3;
            obj_info.objID2 = data_one[i].objID2;
            obj_info.objID4 = data_one[i].objID4;
            obj_info.cameraID1 = data_one[i].cameraID1;
            obj_info.cameraID3 = data_one[i].cameraID3;
            obj_info.cameraID2 = data_one[i].cameraID2;
            obj_info.cameraID4 = data_one[i].cameraID4;
            get_value(&obj_info, &obj_info_T, &data_one[i]);
            data_out.push_back(obj_info);
        }
    }
    //保留下data3剩余的点
    temp_n = data_three.size();
    for (i = 0; i < temp_n; i++) {
        //初始化目标类型
        initvalueNew(&obj_info, angle_value);
        obj_info.objID1 = data_three[i].objID1;
        obj_info.objID3 = data_three[i].objID3;
        obj_info.objID2 = data_three[i].objID2;
        obj_info.objID4 = data_three[i].objID4;
        obj_info.cameraID1 = data_three[i].cameraID1;
        obj_info.cameraID3 = data_three[i].cameraID3;
        obj_info.cameraID2 = data_three[i].cameraID2;
        obj_info.cameraID4 = data_three[i].cameraID4;
        get_value(&obj_info, &obj_info_T, &data_three[i]);
        data_out.push_back(obj_info);
    }
}


void vec_array(vector<OBJECT_INFO_NEW> &data_one, int N, OBJECT_INFO_NEW *data_out) {
    for (int i = 0; i < N; i++)
        data_out[i] = data_one[i];
}

void array_vec(vector<OBJECT_INFO_T> &data_out, int N, OBJECT_INFO_T *data_one) {
    OBJECT_INFO_T obj_info_T;
    for (int i = 0; i < N; i++) {
        obj_info_T = data_one[i];
        data_out.push_back(obj_info_T);
    }
}

void array_vec_NEW(vector<OBJECT_INFO_NEW> &data_out, int N, OBJECT_INFO_NEW *data_one) {
    OBJECT_INFO_NEW obj_info_T;
    for (int i = 0; i < N; i++) {
        obj_info_T = data_one[i];
        data_out.push_back(obj_info_T);
    }
}

//异常值删除
void delete_data1(vector<OBJECT_INFO_T> &data_input, double widthX, double widthY, double Xmax, double Ymax) {
    vector<OBJECT_INFO_T>::iterator iter;

    for (iter = data_input.begin(); iter != data_input.end();)
        if ((abs((*iter).locationX) >= Xmax) || (((*iter).locationY) <= widthY) || (abs((*iter).locationY) >= Ymax))
            iter = data_input.erase(iter);
        else
            iter++;
}

void delete_data2(vector<OBJECT_INFO_T> &data_input, double repateX, double widthX, double widthY, double Xmax,
                  double Ymax) {
    vector<OBJECT_INFO_T>::iterator iter;

    for (iter = data_input.begin(); iter != data_input.end();)
        //可能会有一些靠近雷达的点留下来，进行融合
        if (((((*iter).locationX) <= -repateX) && (((*iter).locationX) >= -widthX)) ||
            (abs((*iter).locationY) >= Ymax) || (abs((*iter).locationX) >= Xmax))
            iter = data_input.erase(iter);
        else
            iter++;
}

void delete_data3(vector<OBJECT_INFO_T> &data_input, double widthX, double widthY, double Xmax, double Ymax) {
    vector<OBJECT_INFO_T>::iterator iter;

    for (iter = data_input.begin(); iter != data_input.end();)
        if ((abs((*iter).locationX) >= Xmax) || (((*iter).locationY) >= -widthY) || (abs((*iter).locationY) >= Ymax))
            iter = data_input.erase(iter);
        else
            iter++;
}

void delete_data4(vector<OBJECT_INFO_T> &data_input, double repateX, double widthX, double widthY, double Xmax,
                  double Ymax) {
    vector<OBJECT_INFO_T>::iterator iter;

    for (iter = data_input.begin(); iter != data_input.end();)
        if (((((*iter).locationX) >= repateX) && (((*iter).locationX) <= widthX)) || (abs((*iter).locationY) >= Ymax) ||
            (abs((*iter).locationX) >= Xmax))
            iter = data_input.erase(iter);
        else
            iter++;
}


//搜索showID
int search_id_single(OBJECT_INFO_NEW data_one, vector<OBJECT_INFO_NEW> &data_before) {
    int temp_data, temp_databefore, N = data_before.size(), j;
    temp_data = data_one.showID;
    for (j = 0; j < N; j++) {
        temp_databefore = data_before[j].showID;
        if (temp_databefore == temp_data)
            return j;
    }
    return -INF;
}


//数据滤波
void Filter_data(vector<OBJECT_INFO_NEW> &before2, vector<OBJECT_INFO_NEW> &before1, vector<OBJECT_INFO_NEW> &now) {
    //before2:前2帧的数据、before1:前1帧的数据、now：当前帧的数据
    int n2 = before2.size(), n1 = before1.size(), n = now.size(), j, temp1, temp2, flag2, flag1;
    for (j = 0; j < n; j++) {
        //寻找id是否存在
        temp2 = search_id_single(now[j], before2);
        if (temp2 != -INF)
            flag2 = 1;
        else
            flag2 = 0;
        temp1 = search_id_single(now[j], before1);
        if (temp1 != -INF)
            flag1 = 1;
        else
            flag1 = 0;
        //根据id情况进行滤波
        //参数可调：哪帧数据不好，权值就小；数据好，权值就大点
        if (flag2) {
            if (flag1) {
                now[j].locationX =
                        0.2 * before2[temp2].locationX + 0.5 * before1[temp1].locationX + 0.3 * now[j].locationX;
                now[j].locationY =
                        0.2 * before2[temp2].locationY + 0.5 * before1[temp1].locationY + 0.3 * now[j].locationY;
            }
        } else {
            if (flag1) {
                now[j].locationX = 0.7 * before1[temp1].locationX + 0.3 * now[j].locationX;
                now[j].locationY = 0.7 * before1[temp1].locationY + 0.3 * now[j].locationY;
            }
        }
    }
}


//哪一帧数据before2没有，就令n_before2=0
int merge_total(double repateX, double widthX, double widthY, double Xmax, double Ymax, double gatetx, double gatety,
                double gatex, double gatey, bool time_flag, OBJECT_INFO_T *Data_one, int n1, OBJECT_INFO_T *Data_two,
                int n2, OBJECT_INFO_T *Data_three, int n3, OBJECT_INFO_T *Data_four, int n4,
                OBJECT_INFO_NEW *Data_before1, int n_before1, OBJECT_INFO_NEW *Data_before2, int n_before2,
                OBJECT_INFO_NEW *Data_out, double angle_value) {
    int N_before, N, i, j, flag, index, used[1000], u0[1000], ubefore[1000], temp, flag1, flag2, tt, delete_n;
    double min_value, tempr, deltaX, deltaY;

    memset(used, 0, sizeof(used));
    memset(u0, 0, sizeof(u0));
    memset(ubefore, 0, sizeof(ubefore));
    vector<OBJECT_INFO_NEW> data_first, data_last, data_out, data_out_new, data_before1, data_before2;
    vector<OBJECT_INFO_T> data_one, data_two, data_three, data_four;

    //接口转换
    array_vec(data_one, n1, Data_one);
    array_vec(data_two, n2, Data_two);
    array_vec(data_three, n3, Data_three);
    array_vec(data_four, n4, Data_four);
    array_vec_NEW(data_before1, n_before1, Data_before1);
    array_vec_NEW(data_before2, n_before2, Data_before2);


    //异常值删除
    delete_data1(data_one, widthX, widthY, Xmax, Ymax);
    delete_data2(data_two, repateX, widthX, widthY, Xmax, Ymax);
    delete_data3(data_three, widthX, widthY, Xmax, Ymax);
    delete_data4(data_four, repateX, widthX, widthY, Xmax, Ymax);

    //对向路口融合
    merge_opposite(data_one, data_three, gatex, gatey, 1, data_first, angle_value);
    merge_opposite(data_two, data_four, gatex, gatey, 2, data_last, angle_value);
    //最后一次融合
    merge_last(data_first, data_last, gatex, gatey, data_out, angle_value);

    //每帧读取上一帧的ID号
    N_before = data_before1.size();
    N = data_out.size();
    if (!time_flag) {
        for (i = 0; i < N_before; i++) {
            temp = data_before1[i].showID;
            used[temp] = 1;
        }

        //按X轴排序
        std::sort(data_out.begin(), data_out.end(), target_loc_compare_new);

        //优先找2个条件都满足的点
        for (i = 0; i < N; i++) {
            index = search_id(&data_out[i], data_before1);
            if ((index != -INF) && (abs(data_out[i].locationX - data_before1[index].locationX) <= gatetx) &&
                (abs(data_out[i].locationY - data_before1[index].locationY) <= gatety)) {
                data_out[i].showID = data_before1[index].showID;
                data_out[i].flag_new = 0;
                u0[i] = 1;
                ubefore[index] = 1;
            }
        }
        //再找只有距离满足的点
        for (i = 0; i < N; i++) {
            if (u0[i] == 0) {
                delete_n = -INF;
                min_value = INF;
                for (j = 0; j < N_before; j++) {
                    if (ubefore[j] == 0) {
                        if ((abs(data_out[i].locationX - data_before1[j].locationX) <= gatetx) &&
                            (abs(data_out[i].locationY - data_before1[j].locationY) <= gatety)) {
                            tempr = sqrt((data_out[i].locationX - data_before1[j].locationX) *
                                         (data_out[i].locationX - data_before1[j].locationX) +
                                         (data_out[i].locationY - data_before1[j].locationY) *
                                         (data_out[i].locationY - data_before1[j].locationY));
                            if (tempr < min_value) {
                                min_value = tempr;
                                delete_n = j;
                            }
                        }
                    }
                }
                if (min_value != INF) {
                    data_out[i].showID = data_before1[delete_n].showID;
                    data_out[i].flag_new = 0;
                    u0[i] = 1;
                    ubefore[delete_n] = 1;
                }
            }
        }
    }
    //未匹配的点全部新建一条航迹
    for (i = 0; i < N; i++) {
        if (u0[i] == 0) {
            for (j = 0; j < 1000; j++) {
                if (used[j] == 0) {
                    data_out[i].showID = j;
                    used[j] = 1;
                    break;
                }
            }
        }
    }
    //删除多余的目标
    for (i = N - 1; i >= 0; i--) {
        flag1 = 1;
        for (j = i - 1; j >= 0; j--) {
            if (data_out[i].showID == data_out[j].showID) {
                flag1 = 0;
                break;
            }
        }
        if (flag1) {
            data_out_new.push_back(data_out[i]);
        }
    }
    N = data_out_new.size();
    //数据滤波
    Filter_data(data_before2, data_before1, data_out_new);

    //接口转换
    vec_array(data_out_new, N, Data_out);
    return N;
}