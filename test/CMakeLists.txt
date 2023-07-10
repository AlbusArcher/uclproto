cmake_minimum_required(VERSION 3.18)

project(test C CXX)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/..)
if (NOT DEFINED THIRD_DIR)
    set(THIRD_DIR ${ROOT_DIR}/third)
endif()
message("THIRD_DIR: ${THIRD_DIR}")

include_directories(SYSTEM ${THIRD_DIR}/include)
link_directories(${THIRD_DIR}/lib)
link_libraries(protoc protobuf ucl)

set(PROTOC_BIN ${THIRD_DIR}/bin/protoc)
set(PROTOC_PLUGIN ${ROOT_DIR}/bin/uclproto)
exec_program(${PROTOC_BIN} ARGS "--plugin=protoc-gen-uclproto=${PROTOC_PLUGIN} --cpp_out=. --uclproto_out=. test.proto")

set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-long-long -fexceptions -g -O2 -fno-omit-frame-pointer -std=c++14 -pipe -fPIC ")

enable_testing()
include(GoogleTest)

file(GLOB TEST_PROTO_SCRS *.pb.cc)
file(GLOB TEST_SCRS *.cpp)

foreach(TEST_FILE ${TEST_SCRS})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WLE)
    add_executable(test_${TEST_NAME} ${TEST_FILE} ${TEST_PROTO_SCRS})
    target_link_libraries(test_${TEST_NAME} PRIVATE gtest gtest_main)
    gtest_add_tests(TARGET test_${TEST_NAME})
endforeach()


