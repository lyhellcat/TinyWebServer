#pragma once

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <functional>
#include <sys/sysinfo.h>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNum=-1) : pool_(std::make_shared<Pool>()) {
        if (threadNum <= 0) {
            threadNum = get_nprocs() + 1;
        }
        for (int i = 0; i < threadNum; i++) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (1) {
                    if (pool->tasks.empty() == false) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if (pool->isStop) {
                        break;
                    } else {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();
        }
    }
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isStop = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class T>
    void AddTask(T&& task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<T>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        bool isStop;
        std::mutex mtx;
        std::condition_variable cond;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};
