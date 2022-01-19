#pragma once

#include <atomic>
#include <cstring>
#include <vector>

class Buffer {
public:
    Buffer(int bufferSize=1024);
    ~Buffer() = default;

private:
    char *BeginPtr_{};
    const char *BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};
