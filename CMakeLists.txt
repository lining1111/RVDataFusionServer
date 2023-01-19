cmake_minimum_required(VERSION 3.19)
project(RVDataFusionServer VERSION 0.0.4)
string(TIMESTAMP COMPILE_TIME ${CMAKE_PROJECT_VERSION}_%Y%m%d_%H.%M.%S)
set(build_time ${COMPILE_TIME})
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_SOURCE_DIR}/version.h")

set(CMAKE_CXX_STANDARD 11)
# 引入Conan的CMake
include(conan/build/conanbuildinfo.cmake)
conan_basic_setup()

#重新定义当前目标的源文件的__FILE__宏
function(redefine_file_macro targetname)
    #获取当前目标的所有源文件
    get_target_property(source_files "${targetname}" SOURCES)
    #遍历源文件
    foreach (sourcefile ${source_files})
        #获取当前源文件的编译参数
        get_property(defs SOURCE "${sourcefile}"
                PROPERTY COMPILE_DEFINITIONS)
        #获取当前文件的绝对路径
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        #将绝对路径中的项目路径替换成空,得到源文件相对于项目路径的相对路径
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        #将我们要加的编译参数(__FILE__定义)添加到原来的编译参数里面
        list(APPEND defs "__FILE__=\"${relpath}\"")
        #重新设置源文件的编译参数
        set_property(
                SOURCE "${sourcefile}"
                PROPERTY COMPILE_DEFINITIONS ${defs}
        )

    endforeach ()
endfunction()

set(LIBS m pthread ${CONAN_LIBS} zlog)

set(SRC_DIRS .
        util
        log
        ringbuffer
        common
        os
        monitor
        signalControl
        server
        client
        server/merge
        simpleini
        db
        getData
        httpServer
        localBussiness
        eoc
        eoc_comm/utility
        eoc_comm)

include_directories(.)
include_directories(eoc_comm/utility)
include_directories(eoc_comm)

set(SRC_FILES)
foreach (src_dir ${SRC_DIRS})
    aux_source_directory(${src_dir} SRC_FILES)
endforeach (src_dir)
list(REMOVE_ITEM SRC_FILES ./main.cpp)


if (${ARCH} STREQUAL "x64")
    include_directories(lib/x64)
    link_directories(lib/x64)
else()
    include_directories(lib/arm64)
    link_directories(lib/arm64)
endif ()

message(STATUS "3.7 SRC_FILES : ${SRC_FILES}")
add_executable(RVDataFusionServer main.cpp ${SRC_FILES})
if (${ARCH} STREQUAL "x64")
    set_target_properties(RVDataFusionServer PROPERTIES LINK_FLAGS "-Wl,-rpath=./lib/x64/")
else ()
    set_target_properties(RVDataFusionServer PROPERTIES LINK_FLAGS "-Wl,-rpath=./lib/arm64/")
endif ()
target_link_libraries(RVDataFusionServer ${LIBS})
redefine_file_macro(RVDataFusionServer)

add_subdirectory(test)