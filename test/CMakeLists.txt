list(TRANSFORM SRC_FILES PREPEND ../)
##testIni
add_executable(testIni testIni.cpp)
target_link_libraries(testIni ${LIBS})
redefine_file_macro(testIni)

##测试通讯
add_executable(testCommon testCommon.cpp ${SRC_FILES})
target_link_libraries(testCommon ${LIBS})
redefine_file_macro(testCommon)

##测试服务端
add_executable(testServer testServer.cpp ${SRC_FILES})
target_link_libraries(testServer ${LIBS})
redefine_file_macro(testServer)

##测试客户端
add_executable(testClient testClient.cpp ${SRC_FILES})
target_link_libraries(testClient ${LIBS})
redefine_file_macro(testClient)

###testGetData
#add_executable(testFusionClient testFusionClient.cpp
#        ../client/FusionClient.cpp
#        ../common/common.cpp
#        ../common/CRC.cpp
#        ../ringbuffer/RingBuffer.cpp
#        ../utility/utility.cpp)
#target_link_libraries(testFusionClient ${LIBS})
#redefine_file_macro(testFusionClient)
#add_executable(testTimeTask testTimeTask.cpp
#        ../os/timeTask.hpp)
#target_link_libraries(testTimeTask ${LIBS})
#redefine_file_macro(testTimeTask)
#
#add_executable(testTempClass testTempClass.cpp)
#target_link_libraries(testTempClass ${LIBS})
#
##测试信号机通讯
add_executable(testSingalControl testSingalControl.cpp
        ../signalControl/signalControlCom.cpp
        ../signalControl/SignalControl.cpp
        ../common/CRC.cpp)
target_link_libraries(testSingalControl ${LIBS})
redefine_file_macro(testSingalControl)

##测试从本地获取数据发送到服务器
add_executable(GetLocalDataSend GetLocalDataSend.cpp ${SRC_FILES})
target_link_libraries(GetLocalDataSend ${LIBS})
redefine_file_macro(GetLocalDataSend)

##旧版参数文件转换为新版
add_executable(algorithmParamExchange algorithmParamExchange.cpp ${SRC_FILES})
target_link_libraries(algorithmParamExchange ${LIBS})
redefine_file_macro(algorithmParamExchange)

##添加启动文件
add_executable(addConfigXML addConfigXML.cpp ${SRC_FILES})
target_link_libraries(addConfigXML ${LIBS})
redefine_file_macro(addConfigXML)

##测试kafka
add_executable(testKafka testKafka.cpp ${SRC_FILES})
target_link_libraries(testKafka ${LIBS})
redefine_file_macro(testKafka)