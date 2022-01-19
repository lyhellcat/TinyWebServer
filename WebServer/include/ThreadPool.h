#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    // explicit ThreadPool(size_t threadNum=-1) :
private:
    struct Pool {
        bool isStop;
        std::mutex mtx;
        std::condition_variable cond;
        // std::queue<>
    };

    std::shared_ptr<Pool> pool_;
};
