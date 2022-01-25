#include <cassert>

#include "sqlconnpool.h"
#include "log.h"
using namespace std;

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

/**
 * @brief Init Sql connection pool
 *
 * @param host
 * @param port
 * @param user
 * @param pwd
 * @param dbName
 * @param connSize Connection pool size, default=10
 */
void SqlConnPool::Init(const char *host, int port, const char *user,
                        const char *pwd, const char *dbName, int connSize=10) {
    assert(connSize > 0);
    // Create coonSize connections
    for (int i = 0; i < connSize; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        if (!mysql_real_connect(sql, host, user, pwd,
                        dbName, port, nullptr, 0)) {
            LOG_ERROR("MySql Connect error!");
            LOG_ERROR("Failed to connect to database: %s", mysql_error(sql));
            continue;
        }
        LOG_INFO("MySql conn: %d Connected!", i);
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() { // Consumer
    MYSQL *sql = nullptr;
    if (connQue_.empty()) {
        LOG_WARN("SqlConnPool busy");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        scoped_lock<mutex> locker(mtx_);
        sql = connQue_.front();
        connQue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL* sql) { // Producer
    assert(sql);
    scoped_lock<mutex> locker(mtx_);
    connQue_.push(sql);
    sem_post(&semId_);
}

void SqlConnPool::ClosePool() {
    scoped_lock<mutex> locker(mtx_);
    // Close all SQL connection
    while (connQue_.empty() == false) {
        MYSQL *sql = connQue_.front();
        connQue_.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() const {
    scoped_lock<mutex> locker(mtx_);
    return connQue_.size();
}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}
