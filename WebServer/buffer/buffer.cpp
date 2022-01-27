#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>

#include "buffer.h"
using namespace std;

Buffer::Buffer(int bufferSize)
    : buffer_(bufferSize) {}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

char* Buffer::BeginPtr_() {
    return &*buffer_.begin();
}

const char* Buffer::ConstBeginPtr_() const {
    return &*buffer_.begin();
}

size_t Buffer::ReadBytes() const {
    return readPos_;
}

void Buffer::InitPtr() {
    memset(&buffer_[0], 0, buffer_.size());
    readPos_ = 0, writePos_ = 0;
}

const char* Buffer::ReadPtr() const {
    return ConstBeginPtr_() + readPos_;
}

char* Buffer::WritePtr() {
    return BeginPtr_() + writePos_;
}

const char* Buffer::ConstWritePtr() const {
    return ConstBeginPtr_() + writePos_;
}

void Buffer::UpdateReadPtr(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::UpdateReadPtrUntilEnd(const char *end) {
    assert(ReadPtr() <= end);
    UpdateReadPtr(end - ReadPtr());
}

string Buffer::RetrieveAllToStr() {
    string str(ReadPtr(), ReadableBytes());
    InitPtr();
    return str;
}

void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

void Buffer::append(const string &str) {
    append(str.data(), str.length());
}

void Buffer::append(const void *data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const char *str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    copy(str, str + len , WritePtr());
    HasWritten(len);
}

void Buffer::append(const Buffer& buff) {
    append(buff.ReadPtr(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
    if (WritableBytes() < len) {
        AllocSpace_(len);
    }
    assert(WritableBytes() >= len);
}

ssize_t Buffer::ReadFd(int fd, int *saveErrno) {
    char buff[1 << 16];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginPtr_() + writePos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    // 使用readv可以分散接收数据
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *saveErrno) {
    ssize_t readSize = ReadableBytes();
    ssize_t len = write(fd, ReadPtr(), readSize);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

void Buffer::AllocSpace_(size_t len) {
    if (WritableBytes() + ReadBytes() < len) {
        // Resize buffer to new length
        buffer_.resize(writePos_ + len + 1);
    } else {
        // 将读指针置为0, 调整buffer
        size_t readable = ReadableBytes();
        copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == ReadableBytes());
    }
}
