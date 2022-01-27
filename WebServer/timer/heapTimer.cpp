#include <cassert>
#include <iostream>

#include "heaptimer.h"
using namespace std;

void HeapTimer::siftUp_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        swapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j])
            j++;
        if (heap_[i] < heap_[j])
            break;
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::addTimer(int fd, int timeout, const TimeoutCallBack& cb) {
    cout << "call add Timer" << endl;
    assert(fd >= 0);
    size_t i;
    if (ref_.count(fd) == 0) {
        // 将新节点插入堆尾, 并调整堆
        i = heap_.size();
        ref_[fd] = i;
        heap_.push_back({fd, Clock::now() + MS(timeout), cb});
        siftUp_(i);
        cout << "push_back: " << heap_.size() << endl;
    } else {
        cout << "已有节点" << endl;
        // 该节点是已有节点, 只需要调整堆
        i = ref_[fd];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if (!siftdown_(i, heap_.size())) {
            siftUp_(i);
        }
    }
}

void HeapTimer::doWork(int fd) {
    cout << "Call Dowork" << endl;
    if (heap_.empty() || ref_.count(fd) == 0) {
        cout << "Not found in heap " << endl;
        return;
    }
    cout << "Found " << endl;
    size_t i = ref_[fd];
    TimerNode node = heap_[i];
    // node.cb();
    del_(i);
    cout << "Do work, " << heap_.size() << endl;
}

void HeapTimer::del_(size_t index) {
    // 删除位于index的节点
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    // 将需要删除的节点交换到堆尾, 调整堆
    size_t i = index;
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        swapNode_(i, n);
        if (!siftdown_(i, n)) {
            siftUp_(i);
        }
    }
    // 从哈希表中移除堆尾的元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::adjust(int fd, int timeout) {
    // 调整index指定的节点
    assert(!heap_.empty() && ref_.count(fd) > 0);
    heap_[ref_[fd]].expires = Clock::now() + MS(timeout);
    siftdown_(ref_[fd], heap_.size());
}

void HeapTimer::tick() {
    // 清除超时结点
    if (heap_.empty()) {
        return;
    }
    // if (heap_.size() != 4090)
    //     cout << heap_.size() << endl;
    while (heap_.size()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now())
                .count() > 0) {
            break;
        }
        node.cb();
        assert(heap_.size());
        pop();
        cout << "In pop, " << heap_.size() << endl;
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

int HeapTimer::GetNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = max(0l, chrono::duration_cast<MS>(heap_.front().expires
                 - Clock::now()).count());
    }
    return res;
}
