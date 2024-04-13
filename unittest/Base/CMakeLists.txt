cmake_minimum_required(VERSION 2.8)
project(serverBase)
set(CMAKE_INSTALL_PREFIX /usr/local)

# 设置路径
SET(PROJECT_TEST_DIR "${PROJECT_SOURCE_DIR}")
SET(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../../src")
SET(PROJECT_DEPS_DIR "${PROJECT_SOURCE_DIR}/../deps")

# 添加宏
ADD_DEFINITIONS(-DPROXY_UNITTEST)
# 设置
add_definitions(-w -g -fno-permissive -fpermissive)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c11 -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fprofile-arcs -ftest-coverage -lgcov")

file(GLOB SOURCE_FILES unittest_main.cpp
${PROJECT_SOURCE_DIR}/*.c)

include_directories(${PROJECT_SOURCE_DIR} 
    ${PROJECT_UT_DIR}
    /usr/local/include
    ${PROJECT_DEPS_DIR}/googletest/googlemock/include/gmock
    ${PROJECT_DEPS_DIR}/googletest/googletest/include/gtest
)

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_DEPS_DIR}/hiredis/libhiredis.a)