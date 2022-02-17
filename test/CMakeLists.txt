##测试通讯
add_executable(testCommon testCommon.cpp
        ../src/common/common.cpp
        ../src/common/CRC.cpp
        ../src/log/Log.cpp)
target_link_libraries(testCommon ${LIBS})
redefine_file_macro(testCommon)