# http连接处理

## select / poll / epoll 如何选择?

- 调用函数
  - select和poll都是一个函数，epoll是一组函数
- 文件描述符数量
  - select每次传给内核一个用户空间分配的fd_set用于表示"关心的socket", 其结构相当于bitset, 限制了只能保存1024个socket. 
  - poll采用链表`poll_list`来维护文件描述符. 函数里可以控制`polldfd`结构的数组大小, 能够突破原来`select`函数最大描述符的限制.
  - epoll底层维护的是红黑树, 监听的文件描述符数为系统可以打开的最大文件描述符数(65535) 

- 将文件描述符从用户传给内核
  - select和poll在每次调用时都需要将文件描述符拷贝到内核态
  - epoll通过epoll_create建立一棵红黑树，通过epoll_ctl将要监听的文件描述符注册到红黑树上

- 内核判断就绪的文件描述符的方式
  - select和poll通过遍历文件描述符集合，判断哪个文件描述符上有事件发生
  - epoll_create时，内核除了帮我们在epoll文件系统里建了个红黑树用于存储以后epoll_ctl传来的fd外，还会再建立一个list链表，用于存储准备就绪的事件，当epoll_wait调用时，仅仅观察这个list链表里有没有数据即可
  - epoll是根据每个fd上面的回调函数(中断函数)判断，只有发生了事件的socket才会主动的去调用callback函数，其他空闲状态socket则不会

- 索引就绪文件描述符
  - `select / poll`只返回发生了事件的文件描述符个数, 需要使用遍历的方式找到发生事件的fd
  - `epoll`返回发生事件的个数和`epoll_event`结构体数组, 里面包含了`data.fd`, 因此直接处理该结构体即可
- 工作模式
  - `select / poll`只能工作在相对低效的ET模式
  - `epoll`工作在ET模式, 并支持`EPOLLONESHOT`事件, 进一步减少 可读/可写/异常 事件被触发的次数
- 应用 
  - 如果所有fd都是活跃连接, `epoll`需要建立红黑树, 效率不如`select / poll`
  - fd数量较少且各个fd都较为活跃, 使用`select / poll`
  - fd数量非常大, 成千上万, 且单位时间内只有部分fd处于就绪状态, 使用`epoll`能明显提升性能

## LT / ET / EPOLLONESHOT

### LT 水平触发模式

(1) `epoll_wait`检测到有事件发生, 则通知应用程序, 应用程序可不立即处理该事件

(2) 当下次调用`epoll_wait`时, 仍会报告此事件, 直至被处理

### ET边缘触发模式

(1) `epoll_wait`检测到事件发生, 通知应用程序, 应用程序需要立即处理这一事件

(2) 使用非阻塞IO处理`read / write`事件, 直到`errno`为`EAGAIN`

### EPOLLONESHOT

可以确保一个socket连接在任一时刻都只能被一个线程处理. 当线程处理完成后需要通过`epoll_ctl`重新设定`EPOLLONESHOT`事件

## HTTP报文

HTTP请求报文由请求行、请求头部、空行和请求数据4部分组成. 

<img src="https://raw.githubusercontent.com/lyhellcat/Pic/master/img/image-20220130115644027.png" alt="image-20220130115644027" style="zoom:80%;" />

请求行由3部分组成: (1) **请求方法**, 常见的为`GET` `POST` 表示对资源的操作 (2) **请求目标**, 通常是URI, 标记了请求方法要操作的资源 (3) **版本号**, 表示报文使用的HTTP协议版本

```http
GET / HTTP/1.1
```

请求行对应于响应报文的状态行, 也包括3部分: (1) 版本号: 表示报文使用的HTTP协议版本 (2) 状态码: 表示请求处理的结果 (3) 原因: 用作状态码的补充说明

```Http
HTTP/1.1 200 OK
```

```http
HTTP/1.1 404 Not found
```

对于错误的HTTP报文, 服务器无法处理: 

```http
HTTP/1.1 400 Bad Request
Server: openresty/1.15.8.1
Connection: close
```

标准的响应报文: 

```http
HTTP/1.1 200 OK
Date: Sun, 30 Jan 2022 12:23:41 +0800
Content-Type: text/html; charset=UTF-8

<html>
	<head></head>
	<body>
		<!--body goes here-->
	</body>
</html>
```

对于一个HTTP连接, 需要维护的信息有: 

```cpp
class HttpConn {
private:
 	int fd_;         // Descriptor for HTTP connection
    bool isClose_;

    struct sockaddr_in addr_;

    int iovCnt_;
    struct iovec iov_[2];  // for writev(), centralized output
    // Read and write buffer
    Buffer readBuff_;
    Buffer writeBuff_;

    HttpRequest request_;
    HttpResponse response_;
};
```



## HTTP请求处理

对于HTTP请求报文, 选择使用有限状态机的模型进行处理, 分为`REQUEST_LINE`, `HEADER` `BODY` `FINISH`4状态进行处理. 

