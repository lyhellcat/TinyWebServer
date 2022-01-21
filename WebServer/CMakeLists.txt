cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
project(Webserver CXX)

set(CXX_FLAGS
 -g
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 -std=c++17
 -rdynamic
)

message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

add_subdirectory(buffer)
add_subdirectory(http)
add_subdirectory(log)
add_subdirectory(net)
add_subdirectory(Server)
add_subdirectory(sql)
add_subdirectory(test)
add_subdirectory(timer)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(Webserver main.cpp)
target_link_libraries(Webserver server)
