//
// Created by lining on 2/15/22.
// RV数据 接收服务端
//

#ifndef _MULTIVIEWSERVER_H
#define _MULTIVIEWSERVER_H

#include <cstdint>
#include <ctime>
#include <vector>
#include <sys/epoll.h>
#include <thread>
#include <atomic>
#include "MultiviewClientInfo.h"
#include "merge/merge.h"

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class MultiviewServer {
public:

public:
    uint16_t port = 9001;//暂定9000
    int maxListen = 5;//最大监听数

    const timeval checkStatusTimeval = {150, 0};//连续150s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};
    const uint8_t thresholdFrame = 100;//不同路时间戳相差门限，单位ms

    int sock = 0;//服务器socket
    //已连入的客户端列表
    vector<MultiviewClientInfo *> vectorClient;
    pthread_mutex_t lockVectorClient = PTHREAD_MUTEX_INITIALIZER;

    //epoll
    int epoll_fd;
    struct epoll_event ev;
#define MAX_EVENTS 1024
    struct epoll_event wait_events[MAX_EVENTS];
    atomic_bool isRun;//运行标志
#define MaxRoadNum 4 //最多有多少路
    int maxQueueTrafficFlows = 30;//最大缓存融合数据量
    Queue<TrafficFlows> queueTrafficFlows;//在同一帧的多路数据

    uint64_t curTimestamp_TrafficFlows = 0;//当前多方向的标定时间戳，即以这个值为基准，判断多个路口的帧是否在门限内。第一次赋值为接收到第一个方向数据的时间戳单位ms
    uint64_t xRoadTimestamp_TrafficFlows[MaxRoadNum] = {0, 0, 0, 0};//多路取同一帧时，第N路的时间戳

    string config = "./config.ini";

    //处理线程
    thread threadMonitor;//服务器监听客户端状态线程
    thread threadCheck;//服务器客户端数组状态线程
    thread threadFindOneFrame_TrafficFlow;//多路数据寻找时间戳相差不超过指定限度的

    string crossID;//路口编号
    string db = "CLParking.db";
    string eocDB = "ecoConfig.db";

    string matrixNo = "0123456789";

    //测试用,存有的速度为0的id
    int idSpeed0 = 0;
    bool isFindSpeed0Id = false;


public:
    MultiviewServer();

    MultiviewServer(uint16_t port, string config, int maxListen = 5);

    ~MultiviewServer();

private:
    /**
     * 从config指定的文件中读取配置参数
     */
    void initConfig();

    void getMatrixNoFromDb();

    void getCrossIdFromDb();

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


    static void ThreadFindOneFrame_TrafficFlow(void *pServer);
};

#ifdef __cplusplus
}
#endif

#endif //_MULTIVIEWSERVER_H
