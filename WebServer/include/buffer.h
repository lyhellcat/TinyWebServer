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
    // ��������д���ֽ���
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t ReadBytes() const;

    // ��ʼ��������, ��ʼ��дָ��ָ�򻺳�����ʼλ��
    void InitPtr();
    const char* ReadPtr() const;
    // ��ȡ��ǰдָ��
    const char* ConstWritePtr() const;
    char* WritePtr();
    // ���¶�ָ��
    void UpdateReadPtr(size_t len);
    // ����ָ����µ�endָ����λ��
    void UpdateReadPtrUntilEnd(const char *end);
    // ������������ת��Ϊ�ַ���
    std::string RetrieveAllToStr();

    // ����ͨ�����仺�����ķ�ʽ��ȷ������Ϊlen������һ��д�뻺����
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);
    // ������д�뻺����
    void append(const std::string& str);
    void append(const char *str, size_t len);
    void append(const void *data, size_t len);
    void append(const Buffer &buff);

    // IO interface
    ssize_t ReadFd(int fd, int &Errno);
    ssize_t WriteFd(int fd, int &Errno);

private:
    // ��������ʼλ��ָ��
    char *BeginPtr_();
    const char *ConstBeginPtr_() const;
    // ���ڻ���������ʱ����buffer
    void AllocSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_{0};
    std::atomic<std::size_t> writePos_{0};
};
