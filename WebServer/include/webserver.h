#pragma once

#include <unordered_map>
#include <arpa/inet.h>

#include "httpconn.h"
#include "heaptimer.h"
#include "Epoll.h"
#include "ThreadPool.hpp"
#include "sqlconnpool.h"

class WebServer {
public:
    WebServer(int port,
              int trigger_mode,
              int timeoutMS,
              bool OptLinger,
              int sqlPort,
              const char* sqlUser,
              const char* sqlPwd,
              const char* dbName,
              int connPoolNum,
              int threadNum,
              bool openLog,
              int logLevel,
              int logQueSize);
    ~WebServer();
    void Run();
    void Stop();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    void ExtentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess_(HttpConn *client);

    static const int MAX_FD = 1 << 16;

    static int SetFdNonblock_(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClosed_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoll> epoll_;
    std::unordered_map<int, HttpConn> users_;
};
