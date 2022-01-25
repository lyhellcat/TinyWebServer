include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_executable(test main.cpp)
target_link_libraries(test my_a my_b)
