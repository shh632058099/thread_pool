cmake_minimum_required(VERSION 3.10)
project(thread_pool VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(thread_pool src/thread_pool.c)
target_include_directories(thread_pool
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>)
target_link_libraries(thread_pool PUBLIC Threads::Threads)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(thread_pool PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(THREAD_POOL_BUILD_TESTS "Build thread pool unit tests" ON)
if(THREAD_POOL_BUILD_TESTS)
    enable_testing()
    add_executable(thread_pool_tests tests/test_thread_pool.c)
    target_link_libraries(thread_pool_tests PRIVATE thread_pool)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(thread_pool_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME thread_pool_tests COMMAND thread_pool_tests)

    add_executable(thread_pool_cpp_compat_tests tests/test_cpp_compat.cpp)
    target_link_libraries(thread_pool_cpp_compat_tests PRIVATE thread_pool)
    add_test(NAME thread_pool_cpp_compat_tests COMMAND thread_pool_cpp_compat_tests)
endif()

add_executable(thread_pool_basic_usage examples/basic_usage.c)
target_link_libraries(thread_pool_basic_usage PRIVATE thread_pool)
