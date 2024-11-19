# xubinh's webserver

## muduo 项目中所采用的抽象

1. `Channel`: 对 socket 文件描述符的抽象, 其中包含事件到来时需要执行的回调函数 (由外部的 Acceptor 或 TcpConnection 类进行注册) 等等.
1. `Buffer`: 管理动态缓冲区, 用于存储 I/O 操作期间 (例如读取客户端发送过来的 HTTP 请求时) 的数据, 优化读写性能.
1. `Acceptor`: 对 listen socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `TcpConnection`: 对 connect socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `Poller`: 对 epoll 机制的抽象.
1. `EventLoop`: 对事件循环的抽象. 每个循环中不仅需要处理 epoll 事件的监听, 还要处理定时器和外部注册到当前线程中的任务等等.
1. `ThreadLocal`: 帮手类, 用于实现 C++11 以前的线程局部存储 (TLS) 机制.
   - C++11 下的等价设施: `thread_local` 关键字
1. `Thread`: 帮手类, 用于实现 C++11 以前的线程机制 (封装了 Linux 的 pthread 库).
   - C++11 下的等价设施: `std::thread`
1. `EventLoopThread`: 对线程 (实际上主要是对工作线程) 的抽象. 是 `EventLoop` 类的一个包装类. 初始化时自动启动一个 `EventLoop`.
1. `EventLoopThreadPool`: 对线程池的抽象. 是 `EventLoopThread` 类的一个包装类.
1. `InetAddress`: 对套接字地址 (IP + port) 的抽象.
1. `TcpServer`: 对服务器的抽象.
1. `TimerQueue`: 对计时器容器的抽象.
1. `Timer`: 对单个计时器元素的抽象.
1. `MutexLock`: 对互斥锁的抽象
   - C++11 下的等价设施: `std::mutex`
1. `MutexLockGuard`: 对互斥锁的 RAII 机制的抽象.
   - C++11 下的等价设施: `std::lock_guard<std::mutex>`
1. `Condition`: 对信号量机制的抽象.
   - C++11 下的等价设施: `std::condition_variable`
1. `Atomic`: 对原子操作的抽象 (但是并不提供内存顺序控制).
   - C++11 下的等价设施: `std::atomic<T>`
1. `BlockingQueue`: 对 (无大小限制的) 阻塞队列的抽象.
1. `BoundedBlockingQueue`: 对 (固定大小的) 阻塞队列的抽象.
1. `Logger`: 是 `AsyncLogging` 类的一个包装类.
1. `AsyncLogging`: 对异步日志系统的抽象.
1. `HttpRequest`: 对 HTTP 请求报文的抽象.
1. `HttpResponse`: 对 HTTP 响应报文的抽象.
1. `HttpContext`: 对 HTTP 请求解析过程的抽象.

## 自己的项目所采用的简化实现的大纲 [TODO]

- `BlockingQueue`: [TODO]
- `LoggingBuffer`: [TODO]
- `AsyncLogging`: [TODO]
- `InetAddress`: [TODO]
- `Channel`: 对文件描述符的抽象, 可能的文件描述符类型包括 listen socket, connect socket, eventfd, timerfd 等等. 提供的接口包括 (1) 在各种事件发生时需要调用的回调函数的注册接口 (由 `Acceptor` 和 `TCPConnection` 等包装类进行设置), 以及 (2) 设置和更改文件描述符的 epoll 监听状态的接口 (和 `Poller` 的关联在创建的时候进行).
- `Acceptor`: [TODO]
- `Poller`: 对 epoll 机制的抽象, 包括创建 epoll 内核事件表, 注册 `Channel` 对象, 维护已经注册的 `Channel` 对象的一个列表, 以及一个 `poll()` 函数用于对 `epoll_wait` 进行封装.
- `TimerContainer`: [TODO]
- `TimerElement`: [TODO]
- `EventLoop`: 用于收纳事件循环相关的代码, 包括事件循环函数主体 (不断调用内部被包装的一个 `Poller` 对象的 `poll()` 函数并执行各个活跃 `Channel` 对象的回调 (此即事件分发))
- `EventLoopThread`: [TODO]
- `EventLoopThreadPool`: [TODO]
- `TcpConnection`: [TODO]
- `TcpServer`: [TODO]
- `HttpBuffer`: [TODO]
- `HttpRequest`: [TODO]
- `HttpResponse`: [TODO]
- `HttpParser`: [TODO]
