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

#ifdef __cplusplus
extern "C"
{
#endif
using namespace std;

class FusionServer {
public:
    typedef struct {
        double timestamp;
        vector<OBJECT_INFO_T> one;
        vector<OBJECT_INFO_T> two;
        vector<OBJECT_INFO_T> three;
        vector<OBJECT_INFO_T> four;
    } OBJS;//同一帧多个路口的数据集合

public:
    uint16_t port = 9000;//暂定9000
    int maxListen = 5;//最大监听数

    const timeval checkStatusTimeval = {150, 0};//连续150s没有收到客户端请求后，断开客户端
    const timeval heartBeatTimeval = {45, 0};
    const uint8_t thresholdFrame = 100;//不同路时间戳相差门限，单位ms

    int sock = 0;//服务器socket
    //已连入的客户端列表
    vector<ClientInfo *> vector_client;
    pthread_mutex_t lock_vector_client = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_vector_client = PTHREAD_COND_INITIALIZER;

    //epoll
    int epoll_fd;
    struct epoll_event ev;
#define MAX_EVENTS 1024
    struct epoll_event wait_events[MAX_EVENTS];
    atomic_bool isRun;//运行标志
#define MaxRoadNum 4 //最多有多少路
    Queue<OBJS> queueObjs;//在同一帧的多路数据
    uint64_t curTimestamp = 0;//当前多方向的标定时间戳，即以这个值为基准，判断多个路口的帧是否在门限内。第一次赋值为接收到第一个方向数据的时间戳单位us
    uint64_t xRoadTimestamp[MaxRoadNum] = {0, 0, 0, 0};//多路取同一帧时，第N路的时间戳

    typedef struct {
        double timestamp;
        vector<OBJECT_INFO_NEW> obj;
    } MergeData;
    Queue<MergeData> queueMergeData;//融合后的数据


    //临时变量，用于融合 输出的物体检测从第1帧开始，上上帧拿上帧的，上帧拿这次输出结果。航向角则是从第2帧开始，上上帧拿上帧的，上帧拿这次输出结果
    int frame = 1;//帧计数
    vector<OBJECT_INFO_NEW> l1_obj;//上帧输出的
    vector<OBJECT_INFO_NEW> l2_obj;//上上帧输出的
    double l1_angle;//上帧输出的
    double l2_angle;//上上帧输出的
    double angle;//角


    //用于融合时的固定变量

    vector<int> roadDirection = {
            West,
            South,
            East,
            North,
    };
    string config = "./config.ini";
    double repateX = 10;//fix 不变
    double widthX = 21.3;//跟路口有关
    double widthY = 20;//跟路口有关
    double Xmax = 300;//固定不变
    double Ymax = 300;//固定不变
    double gatetx = 30;//跟路口有关
    double gatety = 30;//跟路口有关
    double gatex = 10;//跟路口有关
    double gatey = 5;//跟路口有关
    int angle_value = -1000;


    //处理线程
    thread threadMonitor;//服务器监听客户端状态线程
    thread threadCheck;//服务器客户端数组状态线程
    thread threadFindOneFrame;//多路数据寻找时间戳相差不超过指定限度的
    thread threadMerge;//多路数据融合线程

    string crossID;//路口编号
    string db = "CLParking.db";

    string matrixNo = "0123456789";

public:
    FusionServer();

    FusionServer(uint16_t port, string config, int maxListen = 5);

    ~FusionServer();

private:
    /**
     * 从config指定的文件中读取配置参数
     */
    void initConfig();

    void getMatrixNoFromDb();

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
    static void ThreadMerge(void *pServer);
};

#ifdef __cplusplus
}
#endif

#endif //_FUSIONSERVER_H
