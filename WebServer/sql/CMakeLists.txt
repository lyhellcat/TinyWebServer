include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_library(server_sql sqlconnpool.cpp)
target_link_libraries(server_sql mysqlclient)
