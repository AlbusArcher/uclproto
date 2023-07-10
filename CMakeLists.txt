cmake_minimum_required(VERSION 3.18)

project(uclproto C CXX)

set(THIRD_DIR ${PROJECT_SOURCE_DIR}/third)

include_directories(SYSTEM ${THIRD_DIR}/include)
link_directories(${THIRD_DIR}/lib)
link_libraries(protoc protobuf ucl)

if (NOT DEFINED UCL_INCLUDE_DIR)
    set(UCL_INCLUDE_DIR "libucl")
endif()
message("UCL_INCLUDE_DIR: ${UCL_INCLUDE_DIR}")
add_compile_definitions(UCL_INCLUDE_DIR="${UCL_INCLUDE_DIR}")

set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-long-long -fexceptions -g -O2 -fno-omit-frame-pointer -std=c++11 -pipe -fPIC ")

file(GLOB PROJECT_SCRS ${PROJECT_SOURCE_DIR}/src/*.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(${PROJECT_NAME} ${PROJECT_SCRS})

