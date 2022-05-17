# 雷达、视频数据融合服务器
    服务器接收客户端发来的单路雷达、视频数据，并按照时间戳相近且
    1路和3路 2路和4路融合的原则，将两路数据融合成一路数据。发送云平台
    
## 服务器模式
    多客户端，多线程处理，将客户端发送的数据解析后，分别存到各自的数据队列中
    此数据队列应该存在服务器类中，供融合线程来使用

## 本地代码说明
    include src 目录为基本部件目录，内部文件夹名称基本相同 test为测试目录 log为日志目录
    
    common 通信结构目录，包括通信结构体和组包解包静态函数
    
    server 接收多路RV数据，解包、存入接收包队列
    server/ClientInfo 服务端存储的客户端队列个体，包含客户端sock、客户端属性信息、以及客户端接收缓存、分包后的数据等
    server/FusionServer 服务端主体类，建立server sock，监听接入客户端，开启客户端线程、监视客户端状态并作出反应
    
    z_log 基于zlog的日志系统
    
    ringbuffer 环形buffer

    merge 多路融合
    simpleini ini文件读写
    
## 服务端处理流程
    1.客户端主要开启4个线程 threadMonitor threadCheck threadFindOneFrame threadMerge
        1.1
        threadMonitor 服务端 epoll 模式 发现有链接接入时，在accept一个客户端链接后，开启客户端的线程，
        客户端有3个线程：threadDump threadGetPkg threadGetPkgContent
            1.1.1
            threadDump 无脑的把客户端发送的字节数据，存入环形buffer中 rb
            1.1.2
            threadGetPkg 从环形buffer中分包，获取的包内容存入到 queuePkg ,这个队列是加锁的 更新receive_time
            1.1.3
            threadGetPkgContent 从queuePkg 获取包，根据包内的方法名，解包到指定的结构体，存入到指定的队列
            queueWatchData
                    发现有链接断开时，设置标志位 needRelease,供threadCheck使用
        1.2
        threadCheck 本地客户端数组状态检查线程，检查客户端中 receive_time 是否与现在的时间戳相差指定数，如果相差
                    设置needRelease
                    不断检查各个客户端的needRelease，如果为真，
                    且客户端中 rb可读为空，queuePkg没有元素 queueWatchData没有元素，则从客户端数组内删除
        1.3
        threadFindOneFrame 从客户端数组元素中的queueWatchData中，横向寻找延迟在误差范围内的数据，组成一个帧的数据
        OBJS 存入queueObjs队列中
        1.4
        threadMerge 从 queueObjs中取出数据进入融合算法，算出融合后的数据vector<OBJECT_INFO_NEW>,存入queueMergeData
        