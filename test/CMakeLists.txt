# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
        ${CMAKE_BINARY_DIR}/googletest-build)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(Boost 1.58.0 COMPONENTS iostreams date_time filesystem system program_options thread)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(SOURCE_STACK_FILES stack_tests.cpp)
add_executable(stack-test ${SOURCE_STACK_FILES})
target_link_libraries(stack-test zRPC gtest_main)
add_test(NAME stack-test COMMAND stack-test)

set(SOURCE_LIST_FILES list_tests.cpp)
add_executable(list-test ${SOURCE_LIST_FILES})
target_link_libraries(list-test zRPC gtest_main)
add_test(NAME list-test COMMAND list-test)

set(SOURCE_QUEUE_FILES queue_tests.cpp)
add_executable(queue-test ${SOURCE_QUEUE_FILES})
target_link_libraries(queue-test zRPC gtest_main)
add_test(NAME queue-test COMMAND queue-test)

set(SOURCE_TIMER_FILES timer_tests.cpp)
add_executable(timer-test ${SOURCE_TIMER_FILES})
target_link_libraries(timer-test zRPC gtest_main)
add_test(NAME timer-test COMMAND timer-test)

set(SOURCE_RESOLVER_FILES resolver_tests.cpp)
add_executable(resolver-test ${SOURCE_RESOLVER_FILES})
target_link_libraries(resolver-test zRPC gtest_main)
add_test(NAME resolver-test COMMAND resolver-test)

set(SOURCE_FILTER_FILES msgpack_filter_tests.cpp)
add_executable(msgpack-test ${SOURCE_FILTER_FILES})
target_link_libraries(msgpack-test zRPC gtest_main)
add_test(NAME msgpack-test COMMAND msgpack-test)

if(Boost_FOUND)
    target_link_libraries(stack-test ${Boost_LIBRARIES})
    target_link_libraries(list-test ${Boost_LIBRARIES})
    target_link_libraries(queue-test ${Boost_LIBRARIES})
    target_link_libraries(timer-test ${Boost_LIBRARIES})
    target_link_libraries(resolver-test ${Boost_LIBRARIES})
    target_link_libraries(msgpack-test ${Boost_LIBRARIES})
endif()