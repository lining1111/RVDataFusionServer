##测试通讯
add_executable(testCommon testCommon.cpp
        ../src/common/common.cpp
        ../src/common/CRC.cpp)
target_link_libraries(testCommon ${LIBS})