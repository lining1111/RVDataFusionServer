//
// Created by lining on 12/26/22.
//

#ifndef _SIGNALCONTROLCOM_H
#define _SIGNALCONTROLCOM_H

#include <stdint.h>
#include <vector>

using namespace std;

vector<uint8_t> TransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet);

vector<uint8_t> DeTransferMean(vector<uint8_t> in, uint8_t tf, vector<uint8_t> needSet);


/**
 * GBT20999-2017标准规定
 * 物理接口为NET(tcp udp)和
 * RS232(字节结构为一个起始位，八个数据位，一个校验位，一个结束位；接口支持比特率至少包括：1200bit/s、2400bit/s、4800bit/s、9600bit/s和19200bit/s)
 * 通信帧结构
 * 协议约定：
 * ａ） 信号机使用RS-232-C、TCP/IP通信方式时需要使用转义字符，当信号机使用UDP/IP通信方 式时，不需要使用转义字符；
 * ｂ） 协议约定开始字节为0x7E，结束字节为0x7D，转义字符为0x5C，在报文数据中，遇到开始字节、结束字节、转义字符，在其前增加转义字符0x5C；
 * ｃ） 转义字符不参与CRC16校验运算；
 * ｄ） 协议约定采用１６进制方式通信，协议数据为多字节时，高字节在前低字节在后；
 * ｅ） 协议约定读取某个属性或者元素的全部值时，属性或者元素字段填０。
 *
 */


//协议大帧,本地是主机的小端字节，通信的时候为大端字节,这里具体字节按实际情况存，在组织发送字节的时候做多字节转换
class ComFrame_GBT20999_2017 {
public:

    typedef enum{
        Type_Query = 0x10,//查询
        Type_QueryReply = 0x20,//查询应答
        Type_QueryErrorReply = 0x21,//查询出错回复
        Type_Set = 0x30,//设置
        Type_SetReply = 0x40,//设置应答
        Type_SetErrorReply = 0x41,//设置出错回复
        Type_Broadcast = 0x50,//广播
        Type_Trap = 0x60,//主动上报
        Type_HeartSearch = 0x70,//心跳查询
        Type_HeartReply = 0X80,//心跳应答
    }Type;//这里的规律是，回复方的类型为 成功：发送方的类型+0x10;失败：发送方的类型+0x11


    typedef struct {
        uint8_t index;//该条数据值在报文所有操作数据值中的索引位置,从1开始
        uint8_t length;//该条数据值在报文中的长度，包含数据类ID、对象ID、属性ID、数据元素ID、数据值的长度
        uint8_t typeID;//信号机内部数据结构以数据类的形式来组织
        uint8_t objID;//对象是数据类中某一个具体的数据内容的定义
        uint8_t attributeID;//数据对象的具体属性，在查询类型报文中，当属性值为0时，表示读取该对象的所有属性值
        uint8_t elementID;//具体元素的索引，在查询类型报文中，当元素ID为０时，表示读取该属性的 所有元素值
        vector<uint8_t> data;//具体的某个标识号代表的数据或者该标识号的值状态，帧类型为查询类型 时，数据值长度为0
        int getFromBytes(vector<uint8_t> in);
        int setToBytes(vector<uint8_t> &out);
    }DataItem;//数据值集合

    typedef struct {
        uint8_t begin = 0x7e;//开始字节,固定0x7e
        uint16_t length;//报文长度，不包含报文头尾
        uint16_t version;//版本号V1.02  0x0102
        uint8_t controlCenterID;//区分和信号机进行通信的上位机的ID
        uint32_t roadTrafficSignalControllerID;//信号机在整个中心系统中的唯一标识符
        uint8_t roadID;//信号机控制多路口时，通信确认路口序号
        uint8_t sequence;//信号机和上位机之间实时通信时，双方确认通信帧发送的顺序。发送方对 帧流水号进行编码，回复方回复数据采用发送方的流水号值
        uint8_t type;//通信帧的类型,详见 Type
        uint8_t dataItemNum;//该条报文需要操作的数据值的数量，一个报文最大可以操作255个数据值
        vector<DataItem> dataItems;//数据值数组
        uint16_t crc16;//报文数据CRC16校验值，CRC生成多项式x16+x12+x2+1,不包含报文头尾字节
        uint8_t end = 0x7d;//结束字节，固定为0x7d
        int getFromBytes(vector<uint8_t> in);
        int setToBytes(vector<uint8_t> &out);
    } FrameAll;
private:
    bool isUpdateLength = false;
    bool isUpdateCRC = false;
public:
    FrameAll frameAll;
    //更新长度信息
    void UpdateFrameLength();
    //计算CRC16值
    void UpdateFrameCRC16();
    //更新变成字节序列的帧内长度
    static int UpdateByteLength(vector<uint8_t> &bytes);
    //更新变成字节序列的帧内校验值
    static int UpdateByteCRC16(vector<uint8_t> &bytes);

public:
    //从接收的字节流(包含转义字符)内获取本帧内容
    int getFromBytes(vector<uint8_t> in, bool isCheckCRC = true);
    //根据本帧内容形成要发送的字节流(包含转义字符)
    int setToBytes(vector<uint8_t> &out);

};


#endif //_SIGNALCONTROLCOM_H
