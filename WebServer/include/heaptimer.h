/**
 * @file heaptimer.h
 * @author hellcat
 * @brief 为每一个HTTP连接设置一个定时器, 定时清理超过过期时间的连接,
 * 减少服务器的无效资源的耗费
 * @version 0.1
 * @date 2022-01-25
 *
 */

#pragma once

#include <chrono>
#include <functional>
#include <unordered_map>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;             // 定时器id
    TimeStamp expires;  // 定时器过期时间戳
    TimeoutCallBack cb; // 回调函数, 用于删除过期定时器从而关闭HTTP连接
    bool operator<(const TimerNode& t) { return expires < t.expires; }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }

    void adjust(int fd, int newExpires);
    // 添加定时器
    void addTimer(int fd, int timeOut, const TimeoutCallBack &cb);
    // 删除id指定的节点, 并触发回调函数
    void doWork(int fd);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_; // 映射fd在heap_中的index

    void del_(size_t i);       // 删除第i个定时器
    void siftUp_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void swapNode_(size_t i, size_t j);
};
