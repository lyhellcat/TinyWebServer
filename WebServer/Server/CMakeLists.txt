include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_library(server WebServer.cpp)
target_link_libraries(server server_log server_buffer server_sql server_http_request
                    server_http_response server_http_conn server_epoll pthread)
