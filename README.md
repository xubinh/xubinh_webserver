# xubinh's webserver

## muduo 项目中所采用的抽象

> - **下列类型的名称与实际在 muduo 中使用的类型的名称可能并不一致, 因为下列类型是通过 ChatGPT 生成的**.

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

## 自己的项目所采用的简化实现的大纲

- `LoggingBuffer`: 定长缓冲区, 用于写入日志, 不提供额外的函数.
- `AsyncLogging`: 异步日志类, 提供一个 `printf` 函数来进行格式化输出, 内部预先设置一定数量的定长缓冲区 `LoggingBuffer` 对象并构成一个阻塞队列, 并在日志爆满的时候自适应增加新的缓冲区. 此外还设置一个后台线程用于向物理日志文件中输出日志内容, 并且每个一定时间定期输出所收集的日志, 同时每当文件大小超出阈值时便创建新的文件.
- `InetAddress`: 对 socket 地址的抽象, 仅支持 IPv4.
- `Channel`: 对文件描述符的抽象, 可能的文件描述符类型包括 listen socket, connect socket, eventfd, timerfd 等等. 提供的接口包括 (1) 在各种事件发生时需要调用的回调函数的注册接口 (由 `Acceptor` 和 `TCPConnection` 等包装类进行设置), 以及 (2) 设置和更改文件描述符的 epoll 监听状态的接口 (和 `Poller` 的关联在创建的时候进行).
- `Poller`: 对 epoll 机制的抽象, 包括创建 epoll 内核事件表, 注册 `Channel` 对象, 维护已经注册的 `Channel` 对象的一个列表, 以及一个 `poll()` 函数用于对 `epoll_wait` 进行封装.
- `TimerElement`: 对定时器元素的抽象, 每个定时器元素包含一个可随时修改的时间戳和一个无参数无返回值的回调函数.
- `TimerContainer`: 对定时器容器的抽象, 理论上可以实现为有序链表, 时间轮, 以及时间堆等等. 本项目采用时间堆的实现方式.
- `EventLoop`: 用于收纳事件循环相关的代码, 包括事件循环函数主体 (不断调用内部被包装的一个 `Poller` 对象的 `poll()` 函数并执行各个活跃 `Channel` 对象的回调 (此即事件分发)), 通过监听 `timerfd` 定期处理定时器的逻辑, 以及通过监听 `eventfd` 处理外部调用者所分发的回调的逻辑等等, 同时还提供对外的接口用于向定时器容器和回调阻塞队列中添加元素 (即 muduo 中实现的 `runInLoop` 和 `queueInLoop`).
- `EventLoopThread`: 用于创建一个线程并立即开始事件循环.
- `EventLoopThreadPool`: 用于维护一个线程池, 并提供诸如 `draw_a_thread` 等接口函数.
- `TcpBuffer`: 变长缓冲区, 用于存储从 TCP 连接中读取的以及要向 TCP 连接中写入的数据. 数据的读取和写入是发生在 `TcpConnection` 对象为其内含的 `Channel` 对象所设置的回调函数中的.
- `TcpConnection`: 对单个 TCP 连接的抽象, 内含两个 `TcpBuffer` 缓冲区 (一个用于接收, 另一个用于发送) 以及一个 `Channel` 对象. 数据的接收和发送的逻辑位于 `Channel` 对象中, 同时还提供对外的接口用于设置诸如接收到数据时以及数据发送完毕时需要执行的回调等等.
- `HttpContext`: 用于在不连续的数据接收事件之间维护一个逻辑上连续的解析过程. 本对象是存储在 `TcpConnection` 对象中的 (muduo 实现的 `TcpConnection` 对象中设置了一个通用的上下文对象成员 `boost::any context_` 用于存储任意上下文, 也包括 HTTP 上下文 `HttpContext`).
- `HttpRequest`: 对 HTTP 请求报文的抽象, 提供了一些基础的 API, 例如获取请求头等等.
- `HttpResponse`: 对 HTTP 响应报文的抽象, 类似 `HttpRequest`, 同样提供了一些基础的 API.
- `Acceptor`: 对监听并建立客户端连接的过程的抽象, 逻辑上平行于 `TcpConnection` 类. 包括创建 `TcpConnection` 对象, 从线程池中挑选一个工作线程, 以及将 `Channel` 对象注册到该工作线程中的事件循环中等等的逻辑都是在本对象为其包含的 `Channel` 对象所注册的回调函数中执行的.
- `HttpServer`: 对整个服务器的抽象, 集上述所有类型于一体的集大成者. 执行的逻辑包括建立 listen socket, 创建 `Acceptor` 对象并设置回调, 创建并启动线程池, 以及最终的事件循环的启动等等.
