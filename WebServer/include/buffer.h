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
 0      <=      readerIndex   <=   writerIndex    <=     size
*/

class Buffer {
public:
    Buffer(int bufferSize=1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char *end);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* ConstBeginWrite() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const Buffer &buff);

    ssize_t ReadFd(int fd, int &Errno);
    ssize_t WriteFd(int fd, int &Errno);

private:
    char *BeginPtr_();
    const char *ConstBeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_{0};
    std::atomic<std::size_t> writePos_{0};
};
