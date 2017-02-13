cmake_minimum_required(VERSION 3.0)
project(zRPC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

# this one is important
# SET(CMAKE_SYSTEM_NAME Linux)
#
# SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
# SET(CMAKE_C_COMPILER /home/zhsyourai/x-tools/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc)
# SET(CMAKE_CXX_COMPILER /home/zhsyourai/x-tools/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-g++)

# where is the target environment
# SET(CMAKE_FIND_ROOT_PATH /home/zhsyourai/x-tools/arm-unknown-linux-gnu)
#
# search for programs in the build host directories
# SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
# SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(include/)
include_directories(msgpack-c/include)

file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/* ${PROJECT_SOURCE_DIR}/include/*)

add_library(zRPC ${SOURCE_FILES})
target_link_libraries(zRPC pthread dl rt)

enable_testing()
#
add_subdirectory(examples)
add_subdirectory(test)
