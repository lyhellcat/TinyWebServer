#include <cassert>

#include "sqlconnpool.h"
#include "log.h"
using namespace std;

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

void SqlConnPool::Init(const char *host, int port, const char *user,
                        const char *pwd, const char *dbName, int connSize=10) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        if (!mysql_init(sql)) {
            LOG_ERROR("MySql init error");
            LOG_ERROR(mysql_error(sql));
        }
        if (!mysql_real_connect(sql, host, user, pwd, dbName, port, nullptr, 0)) {
            LOG_ERROR("MySql connect error");
            LOG_ERROR(mysql_error(sql));
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if (connQue_.empty()) {
        LOG_WARN("SqlConnPool busy");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        lock_guard<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) {
    assert(sql);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    while (connQue_.empty() == false) {
        MYSQL *sql = connQue_.front();
        connQue_.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
