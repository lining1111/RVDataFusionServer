//
// Created by lining on 2023/3/13.
//

#ifndef NEBULALINK_H
#define NEBULALINK_H
#include "nebulalink.perceptron3.0.5.pb.h"
/**
 * 星云互联
 * 边缘计算单元与路测RSU通信协议V3.0.5
 * 融合计算单元为RSU的南向接入设备，通过RJ45网线进行物理层连接，采用UDP传输协议
 */
#include <string>
#include <mutex>
#include <ringbuffer/RingBuffer.h>
#include <future>
#include <vector>
#include <queue>
#include "Queue.hpp"

namespace Nebulalink {

    using namespace nebulalink::perceptron3;
    /**
    * RSU通信协议
    */
    class RSUCom {
    public:
#pragma pack(1)
        typedef struct{
            uint8_t head[4] = {0xda, 0xdb, 0xdc, 0xdd};
            uint8_t frameType;//0x01:感知数据
            uint8_t perceptionType;
            uint16_t length;//长度
        }Head;
#pragma pack()
        typedef struct {
            Head head;
            std::vector<uint8_t> data;
        public:
            int Pack(std::vector<uint8_t> &data);

            int Unpack(std::vector<uint8_t> data);
        } Frame;
        enum FrameType {
            FrameType_Perception = 0x01,
        };
        enum PerceptionType {
            PerceptionType_Unknown = 0x00,
            PerceptionType_RSU = 0x01,
            PerceptionType_V2XBroadcast = 0x02,
            PerceptionType_Camera = 0x03,
            PerceptionType_MRadar = 0x04,
            PerceptionType_MC = 0x05,
            PerceptionType_LRadar = 0x06,
            PerceptionType_MEC = 0x07,
        };

    };


    /**
     * RSU通信链路
     */
    class RSU {
    private:
        std::string version = "V3.0.5";
        int port = 10086;
        bool isRun = false;
    public:
        std::string ip;
        int sock = 0;
        struct sockaddr_in server_addr;
        std::mutex mtxSend;
#define RecvSize (1024*1024*10)
        RingBuffer *rb = nullptr;
        std::future<int> futureRecv;
        std::future<int> futurePRecv;

        std::vector<PerceptronSet> psSends;
    private:
        /**
         * 创建 udp通信
         * @return udp sock
         */
        int CreateUdpSock();


        /**
         * 接收线程
         * @param p
         * @return
         */
        static int ThreadRecv(void *p);

        enum RecvState{
            SStart = 0,
            SHead,
            SData,
            SEnd,
        };
#define TagLen 4
        std::vector<uint8_t> tag;
        RSUCom::Frame frametmp;
        RecvState recvState;
        /**
         * 接收处理线程
         * @param p
         * @return
         */
        static int ThreadPRecv(void *p);
        Queue<RSUCom::Frame> frames;
        static int ThreadPFrame(void *p);

    public:
        RSU(std::string ip);

        ~RSU();

        bool IsRun();

        /**
       * 发送数据
       * @param data 数据地址
       * @param len 数据长度
       * @return 发送的数据长度
       */
        int Send(uint8_t *data, int len);
    };


}


#endif //NEBULALINK_H
