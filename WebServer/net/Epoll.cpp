#include <cassert>

#include "Epoll.h"

Epoll::Epoll(int maxEvent) : epoll_fd_(epoll_create1(0)),
    events_(maxEvent) {
    assert(epoll_fd_ >= 0 && events_.size() == maxEvent);
}

Epoll::~Epoll() {
    close(epoll_fd_);
}

bool Epoll::AddFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event ev{0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoll::ModifyFd(int fd, uint32_t events) {
    if (fd < 0)
        return false;
    epoll_event ev{0};
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoll::DeleteFd(int fd) {
    if (fd < 0)
        return false;
    epoll_event ev{0};
    return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev) == 0;
}

int Epoll::Wait(int timeoutMS) {
    return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeoutMS);
}

int Epoll::GetEventFd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoll::GetEvents(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}
