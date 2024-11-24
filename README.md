# xubinh's webserver

## 目录

- [待办](#待办)
- [muduo 项目中所采用的抽象](#muduo-项目中所采用的抽象)
- [参考资料](#参考资料)

## 待办

- [x] `AppendOnlyPhysicalFile`: 对物理文件的抽象, 为了提高效率使用了带有一个较大 (64 KB) 用户空间缓冲区的 `fwrite` 进行写入. 之所以是 append file 而不是 write file 是因为 write file 需要额外检查所提供的文件路径是否已经存在, 而检查伴随着决策, 由于决策应该交给外部完成, 因此检查也应该交给外部完成. 提供的 API 尽可能简单, 仅包括两个操作: (1) 追加和 (2) 获取已追加数据的大小.
- [x] `Datetime`: 帮手类, 用于方便地获取当前时间的 epoch 秒数形式或是可读的字符串形式.
- [x] `LogFile`: 对一个大小无限的物理文件流的抽象, 提供的接口基本上和物理文件相同, 只不过在内部维护一个真实的物理文件并在文件大小超过一定阈值的时候透明地切换新文件. 文件流除了需要定期将物理文件的输出缓冲区中的数据刷新至磁盘中以外还需要在物理文件大小达到阈值时或是经过较长的一段时间之后切换新文件.
- [x] `LogBuffer`: 定长缓冲区, 用于收集日志, 仅提供基本的数据写入接口. `LogBuffer` 将会定义为模板类, 其中唯一的模板实参用于指定缓冲区的大小. `LogBuffer` 主要用于两个场景, 一个是在 `LogBuilder` 类对象的内部用于存储正在执行拼接的日志信息片段, 另一个是在 `LogCollector` 类单例对象的内部用于存储各个线程中的 `LogBuilder` 类对象送过来的日志数据.
- [x] `CurrentThread`: 帮手命名空间, 以 thread local 形式缓存线程的 TID (及其字符串形式) 和线程的名称等数据以便加快日志消息的构建.
- [x] `Thread`: 帮手类, 实现对工作线程的惰性启动, 内部封装了底层线程的执行状态以及 TID 等信息, 并提供一系列 API 用于获取这些信息.
- [x] `LogCollector`: 异步日志收集类, 以单例模式实现, 所有需要使用日志功能的线程均通过这个单例对象调用日志功能. 提供一个 `take_this_log_away` 函数来收集日志数据. 内部预先设置一定数量 (2 个即可, 主要是为了将前端接收与后端写入解耦) 的 `LogBuffer` 定长缓冲区对象, 装满的缓冲区构成一个阻塞队列, 并在日志爆满的时候自适应增加新的缓冲区. 任何工作线程在向 `LogCollector` 写入日志前均需要先竞争一个互斥锁. `LogCollector` 还将创建一个后台线程用于定期收集前端接收到的日志并输出至物理文件中.
- [ ] `Format`: 帮手命名空间, 提供从各种常用基本类型的数据统一至字符串的格式化, 包括确定且高效的格式化和使用 `snprintf` 的用户自定义格式化两种风格的手段.
- [ ] `LogBuilder`: 同步日志构建类, 真正用于在各个工作线程中格式化构建日志文本. 每个 `LogBuilder` 对象都是一次性的, 其内部初始化一个小型 `LogBuffer`, 并在构建好单条日志数据之后就地销毁, 这样做的好处是将日志构建保持在栈中进行, 而不是使用堆内存的动态请求与释放 (开销较大). 每条日志文本应当包含 (1) 当前时间, (2) 当前线程的 TID, (3) 当前日志消息的级别, (4) 用户传入的日志上下文文本, (5) 当前源文件名, (6) 当前行号, (7) 当前函数名 (仅当日志级别为 `LOG_TRACE` 或 `LOG_DEBUG` 时), 以及 (8) 当前 `errno` 的值及其解释 (仅当日志级别为 `LOG_SYSERROR` 或 `LOG_SYSFATAL` 时) 等等信息. 日志文本的构建过程分为三层, 第一层是使用预处理宏 `#define` 对日志的级别进行分发, 不同的日志级别可能需要调用 `LogBuilder` 的不同的构造函数 (并传入特定级别下才需要打印的信息), 第二层是在 `LogBuilder` 的构造函数和析构函数中打印除用户传入的日志上下文文本以外的所有日志信息, 第三层是在 `LogBuilder` 类所重载的输出运算符 `operator<<` 中对用户传入的日志上下文数据进行确定且高效的格式化, 并在栈中进行拼接.
- [ ] `InetAddress`: 对 socket 地址的抽象, 仅支持 IPv4.
- [ ] `Channel`: 对文件描述符的抽象, 可能的文件描述符类型包括 listen socket, connect socket, eventfd, timerfd 等等. 提供的接口包括 (1) 在各种事件发生时需要调用的回调函数的注册接口 (由 `Acceptor` 和 `TCPConnection` 等包装类进行设置), 以及 (2) 设置和更改文件描述符的 epoll 监听状态的接口 (和 `Poller` 的关联在创建的时候进行).
- [ ] `Poller`: 对 epoll 机制的抽象, 包括创建 epoll 内核事件表, 注册 `Channel` 对象, 维护已经注册的 `Channel` 对象的一个列表, 以及一个 `poll()` 函数用于对 `epoll_wait` 进行封装.
- [ ] `TimerElement`: 对定时器元素的抽象, 每个定时器元素包含一个可随时修改的时间戳和一个无参数无返回值的回调函数.
- [ ] `TimerContainer`: 对定时器容器的抽象, 理论上可以实现为有序链表, 时间轮, 以及时间堆等等. 本项目采用时间堆的实现方式.
- [ ] `EventLoop`: 用于收纳事件循环相关的代码, 包括事件循环函数主体 (不断调用内部被包装的一个 `Poller` 对象的 `poll()` 函数并执行各个活跃 `Channel` 对象的回调 (此即事件分发)), 通过监听 `timerfd` 定期处理定时器的逻辑, 以及通过监听 `eventfd` 处理外部调用者所分发的回调的逻辑等等, 同时还提供对外的接口用于向定时器容器和回调阻塞队列中添加元素 (即 muduo 中实现的 `runInLoop` 和 `queueInLoop`).
- [ ] `EventLoopThread`: 用于创建一个线程并立即开始事件循环.
- [ ] `EventLoopThreadPool`: 用于维护一个线程池, 并提供诸如 `draw_a_thread` 等接口函数.
- [ ] `TcpBuffer`: 变长缓冲区, 用于存储从 TCP 连接中读取的以及要向 TCP 连接中写入的数据. 数据的读取和写入是发生在 `TcpConnection` 对象为其内含的 `Channel` 对象所设置的回调函数中的.
- [ ] `TcpConnection`: 对单个 TCP 连接的抽象, 内含两个 `TcpBuffer` 缓冲区 (一个用于接收, 另一个用于发送) 以及一个 `Channel` 对象. 数据的接收和发送的逻辑位于 `Channel` 对象中, 同时还提供对外的接口用于设置诸如接收到数据时以及数据发送完毕时需要执行的回调等等.
- [ ] `HttpContext`: 用于在不连续的数据接收事件之间维护一个逻辑上连续的解析过程. 本对象是存储在 `TcpConnection` 对象中的 (muduo 实现的 `TcpConnection` 对象中设置了一个通用的上下文对象成员 `boost::any context_` 用于存储任意上下文, 也包括 HTTP 上下文 `HttpContext`).
- [ ] `HttpRequest`: 对 HTTP 请求报文的抽象, 提供了一些基础的 API, 例如获取请求头等等.
- [ ] `HttpResponse`: 对 HTTP 响应报文的抽象, 类似 `HttpRequest`, 同样提供了一些基础的 API.
- [ ] `Acceptor`: 对监听并建立客户端连接的过程的抽象, 逻辑上平行于 `TcpConnection` 类. 包括创建 `TcpConnection` 对象, 从线程池中挑选一个工作线程, 以及将 `Channel` 对象注册到该工作线程中的事件循环中等等的逻辑都是在本对象为其包含的 `Channel` 对象所注册的回调函数中执行的.
- [ ] `HttpServer`: 对整个服务器的抽象, 集上述所有类型于一体的集大成者. 执行的逻辑包括建立 listen socket, 创建 `Acceptor` 对象并设置回调, 创建并启动线程池, 以及最终的事件循环的启动等等.
- [ ] **确保整个项目是 C++11 兼容的**.

## muduo 项目中所采用的抽象

> - **由于下列类型是通过与 ChatGPT 的问答生成的, 因此可能并不与实际在 muduo 中使用的类型的名称完全一致**.
> - **以下内容仅为个人理解**.

1. `Channel`: 对 socket 文件描述符的抽象, 其中包含事件到来时需要执行的回调函数 (由外部的 Acceptor 或 TcpConnection 类进行注册) 等等.
1. `Buffer`: 管理动态缓冲区, 用于存储 I/O 操作期间 (例如读取客户端发送过来的 HTTP 请求时) 的数据, 优化读写性能.
1. `Acceptor`: 对 listen socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `TcpConnection`: 对 connect socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `Poller`: 对 epoll 机制的抽象.
1. `EventLoop`: 对事件循环的抽象. 每个循环中不仅需要处理 epoll 事件的监听, 还要处理定时器和外部注册到当前线程中的任务等等.
1. `ThreadLocal`: 帮手类, 用于实现 C++11 以前的线程局部存储 (TLS) 机制.
   - C++11 下的等价设施: `thread_local` 关键字
1. `CurrentThread`: 使用 `__thread` 关键字存储一些线程独立的信息, 包括对 TID, TID 的字符串形式, TID 字符串的长度, 以及线程名称字符串的缓存. 同时提供了一些 API 用于初始化以及获取这些信息.
1. `ThreadData`: 帮手类, 用于在启动 `Thread` 对象时在启动 `Thread` 对象的线程 (主线程) 和该对象底层所封装的线程 (工作线程) 之间关于 `Thread` 对象的各个状态成员建立同步关系. 由于状态的改变需要在工作线程中进行, 因此还需要对 `Thread` 对象传进来的可调用对象进一步进行封装, 也就是说封装后的可调用对象会先改变状态, 然后调用内部所封装的原始的可调用对象. `ThreadData` 就是可调用对象的类, 它的成员函数 `runInThread` 和全局作用域中的函数 `startThread` 共同构成了可调用对象的调用运算符.
1. `Thread`: 对线程的抽象, 内部封装了 POSIX pthread API, 并提供一系列额外的简单的 API 用于对底层封装的线程执行各种操作, 包括但不限于 `start`, `stop`, `get_tid` 等等. 使用 `Thread` 最重要的原因是它将底层封装的线程的状态映射到 `Thread` 对象的各个相应成员上, 然后通过一系列 API 方便简洁地获取这些状态信息, 更重要的一点是使用 `Thread` 对象还能够控制线程的生命周期, 实现惰性启动 (如果直接使用 `std::thread` 那么在创建对象的那一刻起底层的线程就已经启动了).
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
1. `AppendFile`: [TODO]
1. `LogFile`: [TODO]
1. `AsyncLogging`: [TODO]
1. `LogStream`: [TODO]
1. `Logger::SourceFile`: [TODO]
1. `Logger::Impl`: [TODO]
1. `Logger`: [TODO]
1. `HttpRequest`: 对 HTTP 请求报文的抽象.
1. `HttpResponse`: 对 HTTP 响应报文的抽象.
1. `HttpContext`: 对 HTTP 请求解析过程的抽象.

## 参考资料

[TODO]
