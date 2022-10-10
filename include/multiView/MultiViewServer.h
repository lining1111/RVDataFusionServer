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
#include "multiView/MultiViewClientInfo.h"
#include "merge/merge.h"

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class MultiViewServer {
public:

public:
    uint16_t port = 9001;//暂定9000
    int maxListen = 5;//最大监听数

    const timeval checkStatusTimeval = {150, 0};//连续150s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};
    const uint8_t thresholdFrame = 100;//不同路时间戳相差门限，单位ms

    int sock = 0;//服务器socket
    //已连入的客户端列表
    vector<MultiViewClientInfo *> vectorClient;
    pthread_mutex_t lockVectorClient = PTHREAD_MUTEX_INITIALIZER;

    //epoll
    int epoll_fd;
    struct epoll_event ev;
#define MAX_EVENTS 1024
    struct epoll_event wait_events[MAX_EVENTS];
    atomic_bool isRun;//运行标志
#define MaxRoadNum 4 //最多有多少路
    queue<TrafficFlows> queueObjs;//在同一帧的多路数据
    pthread_mutex_t lockObjs = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condObjs = PTHREAD_COND_INITIALIZER;
    int maxQueueObjs = 30;//最大缓存融合数据量

    uint64_t curTimestamp = 0;//当前多方向的标定时间戳，即以这个值为基准，判断多个路口的帧是否在门限内。第一次赋值为接收到第一个方向数据的时间戳单位ms
    uint64_t xRoadTimestamp[MaxRoadNum] = {0, 0, 0, 0};//多路取同一帧时，第N路的时间戳

//    typedef struct {
//        double timestamp;
//        vector<OBJECT_INFO_NEW> obj;//融合输出量
//        OBJS objInput;//融合输入量
//    } MergeData;
//    queue<MergeData> queueMergeData;//融合后的数据
//    pthread_mutex_t lockMergeData = PTHREAD_MUTEX_INITIALIZER;
//    pthread_cond_t condMergeData = PTHREAD_COND_INITIALIZER;
//    int maxQueueMergeData = 30;//最大缓存融合数据量


    //临时变量，用于融合 输出的物体检测从第1帧开始，上上帧拿上帧的，上帧拿这次输出结果。航向角则是从第2帧开始，上上帧拿上帧的，上帧拿这次输出结果
//    int frame = 1;//帧计数
//    uint32_t frameCount = 0;//测试用帧计数
//    vector<OBJECT_INFO_NEW> l1_obj;//上帧输出的
//    vector<OBJECT_INFO_NEW> l2_obj;//上上帧输出的
//    double l1_angle;//上帧输出的
//    double l2_angle;//上上帧输出的
//    double angle;//角
//
//
//    //用于融合时的固定变量
//
//    vector<int> roadDirection = {
//            North,//北
//            East,//东
//            South,//南
//            West,//西
//    };
    string config = "./config.ini";

    //处理线程
    thread threadMonitor;//服务器监听客户端状态线程
    thread threadCheck;//服务器客户端数组状态线程
    thread threadFindOneFrame;//多路数据寻找时间戳相差不超过指定限度的
//    thread threadMerge;//多路数据融合线程

//    thread threadNotMerge;//数据不走多路融合，直接给出，id按照东侧的id加10000，西侧的id加20000，北侧的id加30000，南侧的id加40000

    string crossID;//路口编号
    string db = "CLParking.db";
    string eocDB = "ecoConfig.db";

    string matrixNo = "0123456789";

    //测试用,存有的速度为0的id
    int idSpeed0 = 0;
    bool isFindSpeed0Id = false;


public:
    MultiViewServer();

    MultiViewServer(uint16_t port, string config, int maxListen = 5);

    ~MultiViewServer();

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


    static void ThreadFindOneFrame(void *pServer);

    /**
     * 多路数据融合线程
     * @param pServer
     */
//    static void ThreadMerge(void *pServer);
//
//
//    static void ThreadNotMerge(void *pServer);
};

#ifdef __cplusplus
}
#endif

#endif //_MULTIVIEWSERVER_H
