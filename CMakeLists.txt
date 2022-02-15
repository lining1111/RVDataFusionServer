cmake_minimum_required(VERSION 3.19)
project(RVDataFusionServer)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

set(LIBS m pthread zlog jsoncpp)

set(SRC_DIRS src)

foreach (src_dir ${SRC_DIRS})
    aux_source_directory(${src_dir} src_dir_file)
    set(SRC_FILES ${SRC_FILES} ${src_dir_file})
endforeach (src_dir)


add_executable(RVDataFusionServer main.cpp ${SRC_FILES})
target_link_libraries(RVDataFusionServer ${LIBS})