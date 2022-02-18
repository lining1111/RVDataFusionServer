##测试通讯
add_executable(testCommon testCommon.cpp
        ../src/common/common.cpp
        ../src/common/CRC.cpp
        ../src/ringBuffer/RingBuffer.cpp
        ../src/log/Log.cpp)
target_link_libraries(testCommon ${LIBS})
redefine_file_macro(testCommon)

##测试服务端
add_executable(testServer testServer.cpp
        ../src/common/common.cpp
        ../src/common/CRC.cpp
        ../src/ringBuffer/RingBuffer.cpp
        ../src/log/Log.cpp
        ../src/server/ClientInfo.cpp
        ../src/server/FusionServer.cpp
        ../src/merge/merge.cpp)
target_link_libraries(testServer ${LIBS})
redefine_file_macro(testServer)

##测试客户端
add_executable(testClient testClient.cpp
        ../src/common/common.cpp
        ../src/common/CRC.cpp
        ../src/ringBuffer/RingBuffer.cpp
        ../src/log/Log.cpp)
target_link_libraries(testClient ${LIBS})
redefine_file_macro(testClient)