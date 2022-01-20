cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/c++)
project(Webserver CXX)

add_subdirectory(buffer)
add_subdirectory(Server)
add_subdirectory(log)
