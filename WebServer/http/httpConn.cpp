#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include "httpconn.h"
using namespace std;

// Init static variable
const char* HttpConn::srcDir;
atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

/**
 * @brief
 * Init buffer pointer, set isClose_ status to false
 */
void HttpConn::Init(int fd, const sockaddr_in& addr) {
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuff_.InitPtr();
    readBuff_.InitPtr();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(),
             (int)userCount);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (isClose_ == false) {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(),
                 GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET); // ET非阻塞边缘触发需要循环接收数据
    return len;
}

ssize_t HttpConn::write(int *saveErrno) {
    // Write HTTP response line, header, body(file) to fd_
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }

        if (iov_[0].iov_len + iov_[1].iov_len == 0) {  // End of write
            break;
        } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base =
                (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                writeBuff_.InitPtr();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.UpdateReadPtr(len);
        }
    } while (isET || ToWriteBytes() > 10240); // ET模式循环发送数据

    return len;
}

bool HttpConn::Handle() {
    request_.Init();
    if (readBuff_.ReadableBytes() <= 0) {
        return false;
    } else if (request_.parse(readBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_);

    // Write writeBuff_ to response
    iov_[0].iov_base = const_cast<char*>(writeBuff_.ReadPtr());
    iov_[0].iov_len = writeBuff_.ReadableBytes();
    iovCnt_ = 1;

    // Set response file
    if (response_.FileLen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen(), iovCnt_,
              ToWriteBytes());

    return true;
}
