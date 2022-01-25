#pragma once

#include <atomic>
#include <cstring>
#include <vector>
#include <iostream>

/**
 +-------------------+------------------+------------------+
 | prependable bytes |  readable bytes  |  writable bytes  |
 |                   |     (CONTENT)    |                  |
 +-------------------+------------------+------------------+
 |                   |                  |                  |
 0      <=      readPos_   <=   writerIndex    <=     size
*/

class Buffer {
public:
    Buffer(int bufferSize=1024);
    ~Buffer() = default;
    // 缓冲区可写入字节数
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t ReadBytes() const;

    // 初始化缓冲区, 初始读写指针指向缓冲区开始位置
    void InitPtr();
    const char* ReadPtr() const;
    // 获取当前写指针
    const char* ConstWritePtr() const;
    char* WritePtr();
    // 更新读指针
    void UpdateReadPtr(size_t len);
    // 将读指针更新到end指定的位置
    void UpdateReadPtrUntilEnd(const char *end);
    // 将缓冲区数据转化为字符串
    std::string RetrieveAllToStr();

    // 可能通过扩充缓冲区的方式来确保长度为len的数据一定写入缓冲区
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);
    // 将数据写入缓冲区
    void append(const std::string& str);
    void append(const char *str, size_t len);
    void append(const void *data, size_t len);
    void append(const Buffer &buff);

    // IO interface
    ssize_t ReadFd(int fd, int &Errno);
    ssize_t WriteFd(int fd, int &Errno);

private:
    // 缓冲区初始位置指针
    char *BeginPtr_();
    const char *ConstBeginPtr_() const;
    // 用于缓冲区不足时扩容buffer
    void AllocSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_{0};
    std::atomic<std::size_t> writePos_{0};
};
