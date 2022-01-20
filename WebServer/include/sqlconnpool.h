#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>

class SqlConnPool {
public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();

    void Init(const char *host, int port,
            const char *user, const char *pwd,
            const char *dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    int MAX_CONN_;
    int use_count_{0};
    int free_count_{0};

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};
