##testIni
add_executable(testIni testIni.cpp
        ../simpleini/ConvertUTF.c
        )
target_link_libraries(testIni ${LIBS})
redefine_file_macro(testIni)

##测试通讯
add_executable(testCommon testCommon.cpp
        ../common/common.cpp
        ../common/CRC.cpp
        ../ringBuffer/RingBuffer.cpp
        ../log/Log.cpp)
target_link_libraries(testCommon ${LIBS})
redefine_file_macro(testCommon)

##测试服务端
add_executable(testServer testServer.cpp
        ../common/common.cpp
        ../common/CRC.cpp
        ../ringBuffer/RingBuffer.cpp
        ../log/Log.cpp
        ../server/ClientInfo.cpp
        ../server/FusionServer.cpp
        ../client/FusionClient.cpp
        ../merge/merge.cpp
        ../merge/mergeStruct.cpp)
target_link_libraries(testServer ${LIBS})
redefine_file_macro(testServer)

##测试客户端
add_executable(testClient testClient.cpp
        ../common/common.cpp
        ../common/CRC.cpp
        ../ringBuffer/RingBuffer.cpp
        ../log/Log.cpp
        ../getData/GetData.cpp)
target_link_libraries(testClient ${LIBS})
redefine_file_macro(testClient)

##testGetData
add_executable(testGetData testGetData.cpp
        ../getData/GetData.cpp)
target_link_libraries(testGetData ${LIBS})
redefine_file_macro(testGetData)

##testGetData
add_executable(testFusionClient testFusionClient.cpp
        ../client/FusionClient.cpp
        ../common/common.cpp
        ../common/CRC.cpp
        ../log/Log.cpp
        ../ringBuffer/RingBuffer.cpp)
target_link_libraries(testFusionClient ${LIBS})
redefine_file_macro(testFusionClient)
add_executable(testTimeTask testTimeTask.cpp
        ../timeTask.hpp)
target_link_libraries(testTimeTask ${LIBS})
redefine_file_macro(testTimeTask)