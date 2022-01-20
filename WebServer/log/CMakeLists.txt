include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_library(server_log log.cpp)
target_link_libraries(server_log server_buffer pthread)
