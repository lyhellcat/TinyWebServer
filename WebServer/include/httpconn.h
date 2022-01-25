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
    ssize_t read(int &saveErrno);
    ssize_t write(int &saveErrno);
    void Close();
    // 处理HTTP连接, 逻辑为解析request, 生成response
    bool Handle();

    // 获取连接信息的接口
    int GetFd() const;
    int GetPort() const;
    const char* GetIP() const;
    sockaddr_in GetAddr() const;

    int ToWriteBytes() { // 需要写入的数据量
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount; // 所有HTTP连接的个数

private:
    int fd_;         // HTTP连接对应的描述符
    bool isClose_;   // 连接是否被关闭

    struct sockaddr_in addr_;

    int iovCnt_;
    struct iovec iov_[2];
    // 读写缓冲区
    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};
