#pragma once

#include <arpa/inet.h>

#include "log.h"
#include "sqlconnRAII.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);
    ssize_t read(int &saveErrno);
    ssize_t write(int &saveErrno);
    void Close();
    bool process();

    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;

    int ToWriteBytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};
