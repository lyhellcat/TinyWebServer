#include <sys/stat.h>
#include <stdarg.h>

#include "log.h"
using namespace std;

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (deque_->empty() == false) {
            deque_->flush();
        }
        deque_->close();
        writeThread_->join();
    }
    if (fp_) {
        lock_guard<mutex> locker(mtx_);
        flush();
        fclose(fp_);
    }
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}

void Log::Init(int level=1, const char *path,
    const char *suffix, int maxQueueSize) {
    isOpen_ = true;
    level_ = level;
    if (maxQueueSize > 0) {
        isAsync_ = true;
        if (!deque_) {
            unique_ptr<BlockDeque<string>> newDeque(new BlockDeque<string>);;
            deque_ = move(newDeque);
            unique_ptr<std::thread> newThread(new thread(FlushLogThread));
            writeThread_ = move(newThread);
        }
    } else {
        isAsync_ = false;
    }

    lineCount_ = 0;
    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_,
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;

    {
        lock_guard<mutex> locker(mtx_);
        buff_.InitPtr();
        if (fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a");
        if (fp_ == nullptr) {
            mkdir(path_, 0777);
            if ((fp_ = fopen(fileName, "a")) == nullptr) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        // Create new log file
        unique_lock<mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
                 t.tm_mday);

        if (toDay_ != t.tm_mday) {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail,
                     suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        } else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
                     (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        if ((fp_ = fopen(newFile, "a")) == nullptr) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }
    // Write message to log file
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.WritePtr(), 128,
                            "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                            t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        int m = vsnprintf(buff_.WritePtr(), buff_.WritableBytes(), format,
                            vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.append("\n\0", 2);

        if (isAsync_ && deque_ && !deque_->full()) {
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {
            fputs(buff_.ReadPtr(), fp_);
        }
        buff_.InitPtr();
    }

}

void Log::AppendLogLevelTitle_(int level) {
    switch (level) {
        case 0:
            buff_.append("[debug]: ", 9);
            break;
        case 1:
            buff_.append("[info] : ", 9);
            break;
        case 2:
            buff_.append("[warn] : ", 9);
            break;
        case 3:
            buff_.append("[error]: ", 9);
            break;
        default:
            buff_.append("[info] : ", 9);
            break;
    }
}

void Log::flush() {
    if (isAsync_) {
        deque_->flush();
    }
    fflush(fp_);
}

void Log::AsyncWrite_() {
    string str = "";
    while (deque_->pop_front(str)) {
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}
