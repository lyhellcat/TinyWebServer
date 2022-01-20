include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_executable(logTest logTest.cpp)
target_link_libraries(logTest server_log pthread)
