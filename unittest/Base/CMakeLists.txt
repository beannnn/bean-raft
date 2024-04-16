cmake_minimum_required(VERSION 2.8)
project(unittestBase)
set(CMAKE_INSTALL_PREFIX /usr/local)

# 设置路径
set(PROJECT_TEST_DIR "${PROJECT_SOURCE_DIR}")
set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../../src")
set(PROJECT_DEPS_DIR "${PROJECT_SOURCE_DIR}/../deps")

# 添加宏
add_definitions(-DPROXY_UNITTEST)
# 设置
add_definitions(-w -g -fno-permissive -fpermissive)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -fprofile-arcs -ftest-coverage")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c11 -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fprofile-arcs -ftest-coverage -lgcov")

file(GLOB SOURCE_FILES unittest_main.cpp
${PROJECT_SOURCE_DIR}/*.cpp
${PROJECT_SOURCE_DIR}/*.hpp
)

list(REMOVE_ITEM SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/main.cpp"
)

include_directories(${PROJECT_SOURCE_DIR} 
    /usr/local/include
    ${PROJECT_DEPS_DIR}/googletest/googlemock/include/gmock
    ${PROJECT_DEPS_DIR}/googletest/googletest/include/gtest
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} gtest gcov pthread
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/gtest/libgtest_main.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/gtest/libgtest.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/libgmock_main.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/libgmock.a
)