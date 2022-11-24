cmake_minimum_required(VERSION 3.19)
project(RVDataFusionServer VERSION 0.0.2)
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_SOURCE_DIR}/version.h")

set(CMAKE_CXX_STANDARD 11)

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

#根据实际情况设置
#set(ARCH arm)
#set(ARCH x86)
set(ARCH arm64)

if (${ARCH} STREQUAL arm)
    set(CROSS_COMPILE /usr/local/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-)

    set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
    set(CMAKE_C_COMPILER_AR ${CROSS_COMPILE}ar)
    set(CMAKE_C_COMPILER_RANLIB ${CROSS_COMPILE}ranlib)
    set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
endif ()

if (${ARCH} STREQUAL arm64)
    set(CROSS_COMPILE /usr/local/arm64/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-)

    set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
    set(CMAKE_C_COMPILER_AR ${CROSS_COMPILE}ar)
    set(CMAKE_C_COMPILER_RANLIB ${CROSS_COMPILE}ranlib)
    set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
endif ()


add_definitions(-D${ARCH})

set(LIBS m pthread zlog jsoncpp sqlite3 ssl crypto curl)

set(SRC_DIRS ./
        log
        ringBuffer
        common
        monitor
        server
        client
        server/merge
        simpleini
        db
        getData
        httpServer
        eoc
        eoc_comm/utility
        eoc_comm)

include_directories(.)
include_directories(eoc_comm/utility)
include_directories(eoc_comm)


foreach (src_dir ${SRC_DIRS})
    aux_source_directory(${src_dir} src_dir_file)
    set(SRC_FILES ${SRC_FILES} ${src_dir_file})
endforeach (src_dir)


add_executable(RVDataFusionServer main.cpp ${SRC_FILES})
target_link_libraries(RVDataFusionServer libgflags.a ${LIBS})
redefine_file_macro(RVDataFusionServer)

add_subdirectory(test)