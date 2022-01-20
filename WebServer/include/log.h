#pragma once

#include <mutex>
#include <thread>
#include <string>

#include "buffer.h"
#include "BlockDeque.hpp"

class Log {
public:
    void Init(int level, const char *path="./log",
                const char *suffix=".log", int maxQueueCapacity=1024);
    static Log* Instance();
    static void FlushLogThread();

    void write(int level, const char *format, ...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }

private:
    Log()=default;
    ~Log();
    void AppendLogLevelTitle_(int level);
    void AsyncWrite_();

    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;
    int lineCount_{0};
    int level_{0};
    int toDay_{0};

    FILE* fp_{nullptr};
    Buffer buff_{};

    bool isOpen_{false};
    bool isAsync_{false};

    std::unique_ptr<BlockDeque<std::string>> deque_{nullptr};
    std::unique_ptr<std::thread> writeThread_{nullptr};
    std::mutex mtx_;
};

#define LOG_BASE(level, format, ...)                     \
    do {                                                 \
        Log* log = Log::Instance();                      \
        if (log->IsOpen() && log->GetLevel() <= level) { \
            log->write(level, format, ##__VA_ARGS__);    \
            log->flush();                                \
        }                                                \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do {                                   \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do {                                   \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do {                                   \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do {                                   \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);
