cmake_minimum_required(VERSION 3.0)
project(zRPC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++98")

include_directories(include/)
include_directories(msgpack-c/include)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/* ${PROJECT_SOURCE_DIR}/include/*)

add_library(zRPC ${SOURCE_FILES})
target_link_libraries(zRPC pthread dl rt)

enable_testing()
#
add_subdirectory(examples)
add_subdirectory(test)
