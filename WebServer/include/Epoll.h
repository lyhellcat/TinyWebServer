#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

class Epoll {
public:
    explicit Epoll(int maxEvent=4096);
    ~Epoll();

    // File descriptor related operations
    bool AddFd(int fd, uint32_t events);
    bool ModifyFd(int fd, uint32_t events);
    bool DeleteFd(int fd);

    // 返回就绪的文件描述符的个数
    int Wait(int timeoutMS=-1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;

private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};
