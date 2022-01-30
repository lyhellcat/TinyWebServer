#pragma once

#include <arpa/inet.h>

#include "log.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);
    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);
    void Close();
    // Process HTTP connections, the logic is to parse the request and generate
    // the response
    bool Handle();

    // Interface to get connection information
    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;

    // The amount of data that needs to be written
    int ToWriteBytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount; // The number of all HTTP connections

private:
    int fd_;         // Descriptor for HTTP connection
    bool isClose_;

    struct sockaddr_in addr_;

    int iovCnt_;
    struct iovec iov_[2];  // for writev(), centralized output
    // Read and write buffer
    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};
