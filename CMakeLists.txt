cmake_minimum_required(VERSION 3.19)
project(RVDataFusionServer VERSION 0.0.4)
string(TIMESTAMP COMPILE_TIME ${CMAKE_PROJECT_VERSION}_%Y%m%d_%H.%M.%S)
set(build_time ${COMPILE_TIME})
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_SOURCE_DIR}/version.h")

set(CMAKE_CXX_STANDARD 11)
set(-D${ARCH})
# 引入Conan的CMake
if (${ARCH} STREQUAL "x64")
    execute_process(COMMAND sh build_local.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/conan)
else ()
    execute_process(COMMAND sh build_aarch64.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/conan)
endif ()
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

#需要过滤的目录
macro(SUBDIRLISTINCLUDE result curdir depth filtration_dirlist...)
    if (${depth} STREQUAL "1")
        FILE(GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${curdir})
    elseif (${depth} STREQUAL "2")
        FILE(GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${curdir} "${curdir}/*")
    elseif (${depth} STREQUAL "3")
        FILE(GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${curdir} "${curdir}/*" "${curdir}/*/*")
    elseif (${depth} STREQUAL "4")
        FILE(GLOB_RECURSE children LIST_DIRECTORIES true RELATIVE ${curdir} "${curdir}/*" "${curdir}/*/*" "${curdir}/*/*/*")
    else ()
        message(FATAL "depth =[1 2 3 4]")
    endif ()

    #message(DEBUG "\n children: ${children}")
    set(dirlist "")
    foreach (child ${children})
        if ((IS_DIRECTORY ${curdir}/${child}))
            set(flags 0)
            foreach (filtration_dir ${filtration_dirlist}) #过滤目录
                string(FIND ${child} ${filtration_dir} index)
                if (${index} GREATER_EQUAL 0)
                    set(flags 1)
                endif ()
            endforeach ()
            if (${flags} EQUAL 0)
                LIST(APPEND dirlist ${child})
            endif ()
        endif ()
    endforeach ()
    set(${result} ${dirlist})
endmacro()

set(LIBS m pthread ${CONAN_LIBS} zlog)

set(filtration_dirlist cmake-build-debug cmake-build-release-aarch64 .idea .git conan lib test)
SUBDIRLISTINCLUDE(SRC_DIRS ${CMAKE_SOURCE_DIR} 2 ${filtration_dirlist})
message(STATUS "3.7 SRC_DIRS : ${SRC_DIRS}")
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
else ()
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
add_custom_command(TARGET RVDataFusionServer POST_BUILD COMMAND echo ${COMPILE_TIME}>${CMAKE_BINARY_DIR}/version)