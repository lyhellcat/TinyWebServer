/**
 * @file heaptimer.h
 * @author hellcat
 * @brief Ϊÿһ��HTTP��������һ����ʱ��, ��ʱ����������ʱ�������,
 * ���ٷ���������Ч��Դ�ĺķ�
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
    int id;             // ��ʱ��id
    TimeStamp expires;  // ��ʱ������ʱ���
    TimeoutCallBack cb; // �ص�����, ����ɾ�����ڶ�ʱ���Ӷ��ر�HTTP����
    bool operator<(const TimerNode& t) { return expires < t.expires; }
};

class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }
    ~HeapTimer() { clear(); }

    void adjust(int fd, int newExpires);
    // ��Ӷ�ʱ��
    void addTimer(int fd, int timeOut, const TimeoutCallBack &cb);
    // ɾ��idָ���Ľڵ�, �������ص�����
    void doWork(int fd);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_; // ӳ��fd��heap_�е�index

    void del_(size_t i);       // ɾ����i����ʱ��
    void siftUp_(size_t i);
    bool siftdown_(size_t index, size_t n);
    void swapNode_(size_t i, size_t j);
};
