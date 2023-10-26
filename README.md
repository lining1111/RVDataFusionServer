# 雷达、视频数据融合服务器

    服务器接收客户端发来的单路雷达、视频数据，并按照时间戳相近且
    1路和3路 2路和4路融合的原则，将两路数据融合成一路数据。发送云平台

## 服务器模式

    多客户端，多线程处理，将客户端发送的数据解析后，分别存到各自的数据队列中
    此数据队列应该存在服务器类中，供融合线程来使用

## 本地代码说明

    test为测试目录 log为日志目录
    
    common 通信结构目录，包括通信结构体和组包解包静态函数
    os 一些系统操作

    client 客户端主题类
    server 接收多路RV数据，解包、存入接收包队列
    server/ClientInfo 服务端存储的客户端队列个体，包含客户端sock、客户端属性信息、以及客户端接收缓存、分包后的数据等
    server/FusionServer 服务端主体类，建立server sock，监听接入客户端，开启客户端线程、监视客户端状态并作出反应
    server/merge 多路融合
    
    monitor 计算丢包的类
    
    z_log 基于zlog的日志系统 日志等级：notice 类的状态 Info 类内线程的状态 Debug 类内线程内数据的处理状态
    
    ringbuffer 环形buffer

    simpleini ini文件读写
    
    eoc 调用eoc_comm完成eoc通信
    eoc_comm eoc通信
    
    httpServer http通信

## DataUnit说明

    这是一个有多路数据处理特性的数据模板    

    i_queue_vector 多路数据集合，单位为输入数据队列
    o_queue 完成处理流程后的输出数据队列
    fs_ms 单路数据的帧率
    cap 队列的最大容量
    numI i_queue_vector数组的容量
    thresholdFrame 输入数据 各路帧之间的最大时间差
    curTimestamp 处理流程中 标定的时间戳
    xRoadTimestamp 处理流程中 各路取到的时间戳记录
    oneFrame 处理流程中 寻找同一帧后的各路数据

## 信息处理方式说明
    命令字  信息类名                         说明                                          处理方式
    0x03   Fusion                          实时监控数据                                   取各路时间戳相近的做多路融合
    0x04   CrossTrafficJamAlarm            交叉路口堵塞报警                                第一次上传，后面的报警状态与上次不同，则上传
    0x05   AbnormalStopData                异常停车数据                                   透传
    0x06   IntersectionOverflowAlarm       路口溢出报警                                   透传
    0x07   TrafficFlowGather               车流量统计                                     取各路时间戳相近的做flowData集合合并,合并后，按车道号，对排队长度取最大值
    0x08   InWatchData_1_3_4               进口监控数据（1）（3）（4）触发式上报               透传
    0x09   InWatchData_2                   进口监控数据（2）周期上报                         透传
    0x0a   StopLinePassData                停止线过车数据                                  透传
    0x0b   LongDistanceOnSolidLineAlarm    长距离压实线报警                                 透传
    0x0c   HumanData                       3516相机预警信息 行人感知                        单个方向areaList内单元素作和
    0x0d   HumanLitPoleData                人形灯杆数据                                    透传
    0x0e   TrafficDetectorStatus           检测器状态                                      透传

### 20221120

    模板类就是带默认数据类型的通用类
    
    glog 打印等级说明，平时信息INFO VLOG 2网络接收发送内容 3 打印算法内容及网络接收发送内容

### 20230227

    将eoc部分和本地tcp部分分开处理，
    每部分的功能，都分为通信层和业务层，业务层的数据处理做成由数据类区分开来，以此来把程序的耦合分为：
    通信-><-业务-><-处理，三大部分

### 20230830

    通信协议1.1.2,为测试，只有不融合版本

### 20230905

    增加是否存储发送json的开关，实时数据和信控数据分开存储,有图的存图
    路口拥堵报警，只有在报警状态不一致的情况下，才发送报警

### 20230907

    9000服务，专门接实时数据，9001服务，专门接信控数据
    增加程序模式设置，0 起9000 9001服务，1起9000服务，2起9001服务，默认 0

    程序处理方面，在模式下，除了启动相应端口外，还在非0和1模式下，不读取算法配置，初始化算法handle，
    以及接收实时数据(这样就不触发实时数据处理流程)

### 20230908

    检测器状态，做同一帧匹配后，再做合并，合并后，再做报警    

### 20230911

    检测器状态，又改为透传

### 20230914

    增加eoc通信上传主控机信息功能

### 20230915

    修改人形灯杆json错误

### 20230920

    连入客户端数组加锁

### 20230921

    准备增加8路融合版本相关代码
    1、修改算法json文件的结构
    2、和算法沟通新算法json文件的使用
    3、等待算法修改适应8路的库
    4、新程序新库，调试

### 20230922

    修改tcp客户端的异常打印，优化客户端发送异常处理

### 20230928

    sock数据分包缓存增加限制(30)，动态帧率的处理线程启动方式由函数改为线程启动，确保接收线程不会因处理线程的启动而阻塞

### 20231008

    完成eoc下发融合参数功能，并修复eoc重连bug

### 20231010

    增加启动后设置ntp服务地址功能

### 20231017

    新版融合参数结构体修改1

### 20231026

    优化数据相关操作