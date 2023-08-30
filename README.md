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

### 20221120 
    模板类就是带默认数据类型的通用类
    
    glog 打印等级说明，平时信息INFO VLOG 2网络接收发送内容 3 打印算法内容及网络接收发送内容
### 20230227
    将eoc部分和本地tcp部分分开处理，
    每部分的功能，都分为通信层和业务层，业务层的数据处理做成由数据类区分开来，以此来把程序的耦合分为：
    通信-><-业务-><-处理，三大部分
### 20230830
    通信协议1.1.2,为测试，只有不融合版本