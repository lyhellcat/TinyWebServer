#pragma once

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>

class SqlConnPool {
public:
    // Get sql connection pool instance
    static SqlConnPool *Instance();
    // Get connection from pool
    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount() const; // Connection queue's size

    void Init(const char *host, int port,
            const char *user, const char *pwd,
            const char *dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool();

    int MAX_CONN_;
    int used_count_{0};
    int free_count_{0};

    std::queue<MYSQL *> connQue_;  // Ready queue for MySql conn
    mutable std::mutex mtx_;
    sem_t semId_;
};

class SqlConnect {
public:
    SqlConnect(MYSQL **sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }

    ~SqlConnect() {
        if (sql_) {
            connpool_->FreeConn(sql_);
        }
    }

   private:
    MYSQL *sql_;
    SqlConnPool *connpool_;
};
