#pragma once

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

class Epoll {
public:
    explicit Epoll(int maxEvent=1024);
    ~Epoll();

    // File descriptor related operations
    bool AddFd(int fd, uint32_t events);
    bool ModifyFd(int fd, uint32_t events);
    bool DeleteFd(int fd);

    int Wait(int timeoutMS=-1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;

private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};
