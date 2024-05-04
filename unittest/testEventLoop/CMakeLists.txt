cmake_minimum_required(VERSION 2.8)
project(testEventLoop)
#use name of dir as project name
set(CMAKE_INSTALL_PREFIX /usr/local)

if (NOT BUILD_ALL) 
    ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/../Base Base.out)
endif ()

set(PROJECT_TEST_DIR "${PROJECT_SOURCE_DIR}")
set(PROJECT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../../src")
set(PROJECT_DEPS_DIR "${PROJECT_SOURCE_DIR}/../deps")

add_definitions(-DPROXY_UNITTEST)
add_definitions(-w -g -fno-permissive -fpermissive)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11 -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c11 -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fprofile-arcs -ftest-coverage -lgcov")

include_directories(
    ${PROJECT_SOURCE_DIR} 
    /usr/local/include
    ${PROJECT_DEPS_DIR}/googletest/googlemock/include/gmock
    ${PROJECT_DEPS_DIR}/googletest/googletest/include/gtest
)

add_executable(${PROJECT_NAME} TestEventLoop.cpp)

target_link_libraries(${PROJECT_NAME} gtest gcov pthread unittestBase
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/gtest/libgtest_main.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/gtest/libgtest.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/libgmock_main.a
    ${PROJECT_DEPS_DIR}/googletest/build/googlemock/libgmock.a
)

if (NOT BUILD_ALL) 
    add_custom_target(test
        COMMAND ${PROJECT_NAME} --gtest_output=xml:${PROJECT_TEST_DIR}/../output/${PROJECT_NAME}.xml)
endif ()