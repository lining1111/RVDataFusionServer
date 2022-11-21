//
// Created by lining on 2/15/22.
// RV数据 接收服务端
//

#ifndef _FUSIONSERVER_H
#define _FUSIONSERVER_H

#include <cstdint>
#include <ctime>
#include <vector>
#include <sys/epoll.h>
#include <thread>
#include "server/ClientInfo.h"
#include "merge/merge.h"
#include "DataUnit.h"
#include "timeTask.hpp"

using namespace std;

class FusionServer {
public:
    typedef struct {
        string hardCode;//图像对应的设备编号
        vector<OBJECT_INFO_T> listObjs;
        string imageData;
    } RoadData;//路口数据，包含目标集合、路口设备编号、图像数据

    typedef struct {
        double timestamp;
        bool isOneRoad = false;//是否只有一路数据
        int hasDataRoadIndex;
        RoadData roadData[4];
    } OBJS;//同一帧多个路口的数据集合

public:
    uint16_t port = 9000;//暂定9000
    int maxListen = 10;//最大监听数
    bool isMerge = true;

    const timeval checkStatusTimeval = {3, 0};//连续3s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};

    int sock = 0;//服务器socket
    //已连入的客户端列表
    vector<ClientInfo *> vectorClient;
    pthread_mutex_t lockVectorClient = PTHREAD_MUTEX_INITIALIZER;

    //epoll
    int epoll_fd;
    struct epoll_event ev;
#define MAX_EVENTS 1024
    struct epoll_event wait_events[MAX_EVENTS];
    atomic_bool isRun;//运行标志

    map<string,Timer*> timerTasks;

#define MaxRoadNum 4 //最多有多少路

    vector<int> roadDirection = {
            North,//北
            East,//东
            South,//南
            West,//西
    };
    string config = "./config.ini";
    thread threadTimerTask;
    //---------------监控数据相关---------//
    DataUnitFusionData dataUnitFusionData;
    //---------车辆轨迹---------//
    DataUnitMultiViewCarTracks dataUnitMultiViewCarTracks;
    //---------------路口交通流向相关--------//
    DataUnitTrafficFlows dataUnitTrafficFlows;
    //------交叉口堵塞报警------//
    DataUnitCrossTrafficJamAlarm dataUnitCrossTrafficJamAlarm;
    //--------排队长度等信息------//
    DataUnitLineupInfoGather dataUnitLineupInfoGather;


    //处理线程
    thread threadMonitor;//服务器监听客户端状态线程


    string db = "CLParking.db";
    string eocDB = "ecoConfig.db";

    string matrixNo = "0123456789";

    //测试用,存有的速度为0的id
    int idSpeed0 = 0;
    bool isFindSpeed0Id = false;


public:
    FusionServer(bool isMerge);

    FusionServer(uint16_t port, string config, int maxListen, bool isMerge);

    ~FusionServer();

private:
    int getMatrixNoFromDb();
public:
    /**
     * 打开
     * @return 0：success -1：fail
     */
    int Open();

    /**
     * 运行服务端线程
     * @return 0:success -1:fail
     */
    int Run();

    /**
     * 关闭
     * @return 0：success -1：fail
     */
    int Close();

private:
    /**
     * 设置sock 非阻塞
     * @param fd
     * @return 0：success -1:fail
     */
    int setNonblock(int fd);

    /**
     * 向客户端数组添加新的客户端
     * @param client_sock 客户端 sock
     * @param remote_addr 客户端地址信息
     * @return 0：success -1：fail
     */
    int AddClient(int client_sock, struct sockaddr_in remote_addr);

    /**
     * 从客户端数组删除一个指定的客户端
     * @param client_sock 客户端sock
     * @return 0：success -1：fail
     */
    int RemoveClient(int client_sock);

    /**
     * 删除客户端数组成员
     * @return 0
     */
    int DeleteAllClient();

    /**
     * 服务端监听状态监测线程
     * @param pServer
     */
    static void ThreadMonitor(void *pServer);

    /**
     * 服务端存储的客户端数组状态检测线程
     * @param pServer
     */
    static void ThreadCheck(void *pServer);


    static void ThreadTimerTask(void *pServer);

    void addTimerTask(string name, uint64_t timeval_ms, std::function<void()> task);
    void deleteTimerTask(string name);

    void deleteTimerTaskAll();

    static void TaskFusionData(void *pServer, int cache);

    /**
     * 多路数据融合线程
     * @param pServer
     */
    static void ThreadMerge(void *pServer);

    static void ThreadNotMerge(void *pServer);

    static void TaskTrafficFlow(void *pServer, unsigned int cache);

    static void TaskLineupInfoGather(void *pServer, int cache);

    static void TaskCrossTrafficJamAlarm(void *pServer, int cache);

    static void TaskMultiViewCarTracks(void *pServer, int cache);

};

#endif //_FUSIONSERVER_H
