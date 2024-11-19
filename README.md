# xubinh_webserver

## muduo 中采用的抽象

1. `Channel`: 对 socket 文件描述符的抽象, 其中包含事件到来时需要执行的回调函数 (由外部的 Acceptor 或 TcpConnection 类进行注册) 等等.
1. `Buffer`: 管理动态缓冲区, 用于存储 I/O 操作期间 (例如读取客户端发送过来的 HTTP 请求时) 的数据, 优化读写性能.
1. `Acceptor`: 对 listen socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `TcpConnection`: 对 connect socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `Poller`: 对 epoll 机制的抽象.
1. `EventLoop`: 对事件循环的抽象. 每个循环中不仅需要处理 epoll 事件的监听, 还要处理定时器和外部注册到当前线程中的任务等等.
1. `ThreadLocal`: 帮手类, 用于实现 C++11 以前的线程局部存储 (TLS) 机制.
1. `Thread`: 帮手类, 用于实现 C++11 以前的线程机制 (封装了 Linux 的 pthread 库).
1. `EventLoopThread`: 对线程 (实际上主要是对工作线程) 的抽象. 是 `EventLoop` 类的一个包装类. 初始化时自动启动一个 `EventLoop`.
1. `EventLoopThreadPool`: 对线程池的抽象. 是 `EventLoopThread` 类的一个包装类.
1. `InetAddress`: 对套接字地址 (IP + port) 的抽象.
1. `TcpServer`: 对服务器的抽象.
1. `TimerQueue`: 对计时器容器的抽象.
1. `Timer`: 对单个计时器元素的抽象.
1. `MutexLock`: 对互斥锁的抽象
1. `MutexLockGuard`: 对互斥锁的 RAII 机制的抽象.
1. `Condition`: 对信号量机制的抽象.
1. `Atomic`: 对原子操作的抽象 (但是并不提供内存顺序控制).
1. `CountDownLatch`: 对倒计时计数器 (或称闭锁) 的抽象.
1. `BlockingQueue`: 对 (无大小限制的) 阻塞队列的抽象.
1. `BoundedBlockingQueue`: 对 (固定大小的) 阻塞队列的抽象.
1. `Logger`: 是 `AsyncLogging` 类的一个包装类.
1. `AsyncLogging`: 对异步日志系统的抽象.
1. `HttpRequest`: 对 HTTP 请求报文的抽象.
1. `HttpResponse`: 对 HTTP 响应报文的抽象.
1. `HttpContext`: 对 HTTP 请求解析过程的抽象.

## 大纲 [TODO]

- **通用**: 使用智能指针等 RAII 机制尽可能减少内存泄漏.
- **线程同步机制包装类**: 对 Linux 中提供的 POSIX 互斥锁和条件变量原语进行封装 (此处不需要包装信号量, 因为信号量完全可以由条件变量进行实现).
- **阻塞队列**: 结合线程同步机制包装类实现 (有限大小的) 阻塞队列.
- **异步日志模块**: 以单例模式实现, 并结合阻塞队列实现异步输出日志至文件.
- **线程池**: 结合阻塞队列实现.
- **HTTP 连接类**: 对 HTTP 客户端连接进行封装, 包括 HTTP 请求的解析, 服务器以零拷贝方式读取本地文件, 以及 HTTP 响应的生成三个部分. 因为需要和线程池进行联动, 因此还需要实现线程池所要求的 API. 还需实现一次性连接与长连接 (keep-alive 选项).
- **定时器**: 具有三个不同层次的实现, 分别为最外部的定时器的实现, 中间层的不同定时器容器的实现, 以及最内层的定时器元素的实现. 其中定时器容器应当最为解耦, 功能应当最为简单. 定时器元素和定时器之间的关系应当最紧密. 定时器容器的默认实现为时间堆, 同时采用惰性删除.
- **WebServer 服务器类**: 对服务器的封装, 采用高效的半同步/半异步模式, 也就是说主线程通过一个 NIO selector 监听并接受客户发起的连接, 然后采用 Round Robin 方式将客户连接分发给工作线程 (此时是主线程使用 eventfd 推送文件描述符并顺便唤醒工作线程, 而不是像原先那样所有工作线程在同一个互斥锁上轮询阻塞队列), 每个工作线程同样通过一个 NIO selector 进行 I/O 事件和数据的处理. 利用定时器定期关闭超时请求和不活跃连接以提高服务器运行效率. 利用单独的信号线程实现事件源的统一. 后期可以考虑升级至优雅关闭 (gracefully shutting down) 连接.
