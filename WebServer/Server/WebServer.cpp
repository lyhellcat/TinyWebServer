#include "webserver.h"

using namespace std;

WebServer::WebServer(int port, int trigger_mode, int timeoutMS, bool is_open_linger,
                     int sqlPort, const char* sqlUser, const char* sqlPwd,
                     const char* dbName, int connPoolNum, int threadNum,
                     bool isOpenLog, int logLevel, int logQueSize)
    : port_(port),
      openLinger_(is_open_linger),
      timeoutMS_(timeoutMS),
      isClosed_(false),
      timer_(new HeapTimer()),
      threadpool_(new ThreadPool(threadNum)),
      epoll_(new Epoll()) {
    // Set the resource file directory
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/../resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;

    if (isOpenLog) {
        Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if (isClosed_) {
            LOG_ERROR("========== Server init error!==========");
        } else {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port_,
                     is_open_linger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent_ & EPOLLET ? "ET" : "LT"),
                     (connEvent_ & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum,
                     threadNum);
        }
    }
    InitEventMode_(trigger_mode);
    if (!InitSocket_()) {
        isClosed_ = true;
        LOG_ERROR("Init socket failed");
        exit(1);
    }
    LOG_INFO("============== Create SqlConnPool =================");
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName,
                                  connPoolNum);
}

void WebServer::Stop() {
    close(listenFd_);
    isClosed_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

WebServer::~WebServer() {

}

void WebServer::InitEventMode_(int trigger_mode) {
    // EPOLLRDHUP can trigger an event when the peer is closed
    // EPOLLONESHOT disable checking after event notification is complete
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    // Set trigger mode, EPOLLET adopts edge trigger event notification
    // The default state is horizontal trigger mode
    switch (trigger_mode) {
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        case 0:
        default:
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

/**
 * @brief Init socket for webserver, including graceful shutdown,
 *  port reuse and other settings.
 * socket() -> setsockopt() -> bind() -> listen() -> Add listenFd to epoll
 * -> set listenFd non block (Using non block IO model)
 * @return true if init socket success
 */
bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port number:%d error!", port_);
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = {0};

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    if (openLinger_) {
        // Until the remaining data is sent or timed out
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    // Set TCP connection to be closed gracefully or rudely
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger,
                     sizeof(optLinger));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    // Port multiplexing
    // Only the last socket will receive data normally.
    int on = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoll_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonblock_(listenFd_);
    LOG_INFO("Init Server socket, succuss in port[%d]", port_);

    return true;
}

void WebServer::Run() {
    int timeMS = -1;
    if (!isClosed_) {
        LOG_INFO("========== Server start ==========");
    }
    while (!isClosed_) {
        if (timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoll_->Wait(timeMS);
        for (int i = 0; i < eventCnt; i++) {
            // Deal event
            int fd = epoll_->GetEventFd(i);
            uint32_t events = epoll_->GetEvents(i);
            if (fd == listenFd_) {
                DealListen_();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                CloseConn_(&users_[fd]);
            } else if (events & EPOLLIN) {
                DealRead_(&users_[fd]);
            } else if (events & EPOLLOUT) {
                DealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::CloseConn_(HttpConn* client) {
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoll_->DeleteFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeoutMS_ > 0) {
        timer_->addTimer(fd, timeoutMS_,
                    bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoll_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock_(fd);
    LOG_INFO("Client[%d] in!", fd);
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr*)&addr, &len);
        if (fd <= 0) {
            // accept() on success should return nonnegative int
            return;
        } else if (HttpConn::userCount >= MAX_FD) {
            LOG_WARN("Clients is full, server busy");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn *client) {
    assert(client);
    // ExtentTime_(client);
    threadpool_->AddTask(bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client) {
    assert(client);
    // ExtentTime_(client);
    threadpool_->AddTask(bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client) {
    assert(client);
    if (timeoutMS_ > 0) {
        timer_->adjust(client->GetFd(), timeoutMS_);
    }
}

void WebServer::OnRead_(HttpConn *client) {
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess_(client);
}

void WebServer::OnProcess_(HttpConn *client) {
    if (client->Handle()) {
        epoll_->ModifyFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoll_->ModifyFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn* client) {
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->ToWriteBytes() == 0) {
        // Transfer completed
        if (client->IsKeepAlive()) {
            OnProcess_(client);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            // Continue transfer
            epoll_->ModifyFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

int WebServer::SetFdNonblock_(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}
