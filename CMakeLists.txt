cmake_minimum_required(VERSION 3.10)
project(RVDataFusionServer VERSION 1.3.2)
string(TIMESTAMP COMPILE_TIME ${CMAKE_PROJECT_VERSION}_%Y%m%d_%H.%M.%S)
set(build_time ${COMPILE_TIME})
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_SOURCE_DIR}/version.h")

set(CMAKE_CXX_STANDARD 11)
add_definitions(-D${ARCH})
# 引入Conan的CMake
set(CONAN_DISABLE_CHECK_COMPILER 1)
if (${ARCH} STREQUAL "x64")
    execute_process(COMMAND sh build_local.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/conan)
else ()
    execute_process(COMMAND sh build_aarch64.sh
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/conan)
endif ()
include(conan/build/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
include(utility)
include(CPM)

set(LIBS m pthread ${CONAN_LIBS})

set(filtration_dirlist cmake-build-debug cmake-build-release-aarch64 .idea .git conan lib test nebulalink)
SUBDIRLISTINCLUDE(SRC_DIRS ${CMAKE_SOURCE_DIR} 2 ${filtration_dirlist})
message(STATUS "3.7 SRC_DIRS : ${SRC_DIRS}")
include_directories(.)
include_directories(eoc_comm/utility)
include_directories(eoc_comm)

if (${ARCH} STREQUAL "x64")
    include_directories(lib/x64)
    file(GLOB allCopyFiles "lib/x64/*.so" "lib/x64/*.so.*" "lib/x64/*.a")
    file(COPY ${allCopyFiles} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib FOLLOW_SYMLINK_CHAIN)
else ()
    include_directories(lib/arm64)
    file(GLOB allCopyFiles "lib/arm64/*.so" "lib/arm64/*.so.*" "lib/arm64/*.a")
    file(COPY ${allCopyFiles} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib FOLLOW_SYMLINK_CHAIN)
endif ()
link_directories(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/lib)
set(CMAKE_CXX_FLAGS "-Wl,-rpath=lib")

foreach (src_dir ${SRC_DIRS})
    aux_source_directory(${src_dir} SRC_FILES)
endforeach (src_dir)
list(REMOVE_ITEM SRC_FILES ./main.cpp)

add_executable(RVDataFusionServer main.cpp ${SRC_FILES})

target_link_libraries(RVDataFusionServer ${LIBS})
redefine_file_macro(RVDataFusionServer)

add_subdirectory(test)
add_custom_command(TARGET RVDataFusionServer POST_BUILD COMMAND echo ${COMPILE_TIME}>${CMAKE_BINARY_DIR}/version)