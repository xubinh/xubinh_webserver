# xubinh's webserver

## 目录

- [待办](#待办)
- [muduo 项目中所采用的抽象](#muduo-项目中所采用的抽象)
- [参考资料](#参考资料)

## 待办

- [x] `AppendOnlyPhysicalFile`: 对物理文件的抽象.
  - 为了提高效率使用了带有一个较大 (64 KB) 用户空间缓冲区的 `fwrite` 进行写入.
  - 之所以是 append file 而不是 write file 是因为 write file 需要额外检查所提供的文件路径是否已经存在, 而检查伴随着决策, 由于决策应该交给外部完成, 因此检查也应该交给外部完成. 由于不需要检查所以直接使用 append file 即可.
  - 提供的 API 尽可能简单, 仅包括两个操作:
    1. 追加数据.
    1. 获取已追加数据的大小.
- [x] `Datetime`: 帮手类, 用于方便地获取当前时间的 epoch 秒数形式或是可读的字符串形式.
- [x] `LogFile`: 对一个大小无限的物理文件流的抽象.
  - 提供的 API 基本上和物理文件相同, 只不过在内部维护一个真实的物理文件并在文件大小超过一定阈值的时候透明地切换新文件.
  - 文件流除了需要定期将物理文件的输出缓冲区中的数据刷新至磁盘中以外, 还需要在物理文件大小达到阈值时或是经过较长的一段时间之后切换新文件.
- [x] `LogBuffer`: 定长缓冲区, 用于收集日志.
  - 仅提供基本的数据写入接口.
  - `LogBuffer` 将会定义为模板类, 其中唯一的模板实参用于指定缓冲区的大小.
  - `LogBuffer` 主要用于两个场景:
    1. 在 `LogBuilder` 类对象的内部用于存储正在执行拼接的日志信息片段.
    1. 在 `LogCollector` 类单例对象的内部用于存储各个线程中的 `LogBuilder` 类对象送过来的日志数据.
- [x] `current_thread`: 帮手命名空间, 用于管理当前线程的元数据.
  - 主要原理是利用 `thread_local` 关键字创建线程局部变量来缓存线程的 TID (及其字符串形式) 和线程的名称等数据以便加快日志消息的构建.
- [x] `Thread`: 帮手类, 对线程的抽象.
  - 接受一个由外部调用者传入的通用的 (无参数且无返回值的) 回调并将其存储在内部, 提供 API 实现对线程的惰性启动.
  - 内部封装了底层线程的执行状态以及 TID 等信息, 并提供对应的 API 用于获取这些信息.
- [x] `LogCollector`: 异步日志收集类, 以单例模式实现.
  - 所有需要使用日志功能的线程均通过这个单例对象调用日志功能.
  - 提供一个 `take_this_log_away` 函数来收集日志数据.
  - 内部预先设置一定数量 (2 个即可, 主要是为了将前端接收与后端写入解耦) 的 `LogBuffer` 定长缓冲区对象, 装满的缓冲区构成一个阻塞队列, 并在日志爆满的时候自适应增加新的缓冲区.
  - 任何工作线程在向 `LogCollector` 写入日志前均需要先竞争一个互斥锁.
  - `LogCollector` 还将创建一个后台线程用于定期收集前端接收到的日志并输出至物理文件中.
- [x] `type_traits`: 帮手命名空间, 用于存放 SFINAE 相关的基础设施.
- [x] `Format`: 帮手类, 用于存放格式化相关的一系列常用函数.
  - 相关功能包括:
    - 编译期获取各种基本类型 (整数, 浮点, 以及指针等等) 变量的最大占用字节数.
    - 从各种常用基本类型的数据统一至字符串的确定且高效的格式化函数.
    - 编译期获取路径的最右端不包含斜杠 `/` 的部分.
- [x] `LogBuilder`: 同步日志构建类, 真正用于在各个工作线程中格式化构建日志文本.
  - 所有可能的日志等级归纳如下:
    - `LOG_TRACE`: 粒度最小, 用于跟踪执行堆栈的轨迹. 例如函数体中的递归执行轨迹等.
    - `LOG_DEBUG`: 粒度较小, 用于打印调试所需的中间信息. 例如进入函数体前各个实参的值.
    - `LOG_INFO`: 正常粒度, 用于标记程序的执行状态的阶段性变化.
    - `LOG_WARN`: 一般错误, 所得结果与预期结果不符但不影响程序的执行.
    - `LOG_ERROR`: 严重错误, 一定程度上影响程序的正常执行.
    - `LOG_FATAL`: 致命错误, 程序已经无法继续执行, 整个进程应当立即终止.
    - `LOG_SYS_ERROR`: 同 `LOG_ERROR`, 但错误源自系统调用.
    - `LOG_SYS_FATAL`: 同 `LOG_FATAL`, 但错误源自系统调用.
  - 每个 `LogBuilder` 对象都是一次性的, 其内部初始化一个小型 `LogBuffer`, 并在构建好单条日志数据之后就地销毁. 这样做的好处是将日志构建保持在栈中进行, 而不是使用堆内存的动态请求与释放 (开销较大).
  - 每条日志文本应当包含:
    1. 当前时间.
    1. 当前线程的 TID.
    1. 当前日志消息的级别.
    1. 用户传入的日志上下文文本.
    1. 当前源文件名.
    1. 当前行号.
    1. 当前函数名 (仅当日志级别为 `LOG_TRACE` 或 `LOG_DEBUG` 时).
    1. 当前 `errno` 的值及其解释 (仅当日志级别为 `LOG_SYS_ERROR` 或 `LOG_SYS_FATAL` 时).
  - 日志文本的构建过程分为三层:
    - 第一层是使用预处理宏 `#define` 对日志的级别进行分发, 不同的日志级别可能需要调用 `LogBuilder` 的不同的构造函数 (并传入特定级别下才需要打印的信息).
    - 第二层是在 `LogBuilder` 的构造函数和析构函数中打印除用户传入的日志上下文文本以外的所有日志信息.
    - 第三层是在 `LogBuilder` 类所重载的输出运算符 `operator<<` 中对用户传入的日志上下文数据进行确定且高效的格式化, 并在栈中进行拼接.
- [x] `BlockingQueue`: 阻塞队列.
- [ ] `FunctorQueue`: 函子专用的阻塞队列, 用于 `EventLoop` 类中作为其成员.
- [ ] `TimeInterval`: 高精度时间区间, 精度为毫秒 ($10^{-6}$ 秒). 具体实现其实就是 64 位定长整型 `int64_t` 的别名.
- [ ] `TimePoint`: 高精度时间点, 基于 `TimeInterval` 实现, 精度为毫秒.
  - 一个 `TimePoint` 对象所代表的时间点就是这个对象的创建时间.
  - 一个时间点对象在创建之后可以和时间区间对象进行加减运算.
- [ ] `Timer`: 对定时器元素的抽象.
  - 一个定时器元素包含的成员如下:
    - 一个代表触发时间的时间点 `TimePoint` 对象.
    - 一个代表重复触发的间隔时间的时间区间 `TimeInterval` 对象. 如果其值为 `0` 则表示一次性定时器.
    - 一个代表重复次数的 `int` 类型对象. 如果其值为 `-1` 则表示无限重复, 如果其值为非负数则表示本次执行之后仍需重复的次数.
    - 一个 (无参数且无返回值的) 回调函数.
- [ ] `TimerContainer`: 对定时器容器的抽象. 内部采用树的实现方式 (`std::multimap`).
  - 关于实现方式的思考:
    - 一种实现方式是使用堆, 这样做不仅一般操作的时间复杂度较低 (插入操作的时间复杂度为 $O(\log{n})$, 弹出为 $O(1)$), 而且不需要维护树形结构, 只需要一个静态数组即可, 较为节省内存资源. 但其有一个重要缺点是不支持随机删除 (虽然也可以实现, 但是会引入额外的内存消耗并降低执行速度, 抵消掉使用堆带来的好处).
    - 另一种实现方式是使用树, 通过维护有序的树形结构来支持随机删除, 用空间省时间, 其他方面和堆相同.
      - 定时器容器至少需要对定时器关于时间点进行排序. 由于多个不同的定时器可能在同一个时间点下超时, 并且一般情况下不需要关于定时器本身定义全序关系, 所以内部实现选择 `std::multimap` 会比较合适. 
      - muduo 选择 `std::set<std::pair<K, V>>` (实际上就是 `std::map<K, V>`) 作为内部实现, 这就涉及到对定时器指针类型 (`V`) 的排序, 虽然从硬件的角度看这是完全没问题的 (指针的值就是一个无符号整型, 因此比较是肯定可以比较的), 但是从逻辑上 (不论是 [C++11 标准](https://stackoverflow.com/questions/9086372/how-do-you-check-if-two-pointers-point-to-the-same-object)还是定时器容器的抽象本身) 来看是没必要的.
  - 内部实现为以 `TimePoint` 时间点为 key, `Timer` 对象指针为值的
  - 提供的 API:
    - 注册一个定时器.
    - 取消一个定时器.
    - 获取在给定时间点前超时的所有定时器.
  - 还需要提供互斥锁来确保上述 API 的原子性.
- [ ] `EventDispatcher`: 为各类可供监听事件的文件描述符提供统一的事件分发的接口.
  - Linux 下可供监听事件的文件描述符包括:
    - 监听套接字 (listen socket)
    - 连接套接字 (connect socket)
    - 事件描述符 (eventfd)
    - 定时器描述符 (timerfd)
    - 信号描述符 (signalfd)
  
    框架内部不使用信号描述符, 而是留给用户自己注册想要的回调. 唯一的例外是 `SIGPIPE` 信号, 框架内部必须屏蔽该信号以便后续将其整合到事件循环的处理逻辑中 (即写入错误码 `EPIPE`).
  
  - 由于事件的注册需要用到 `epoll_event` 类型的一个对象, 因此内部需要维护一个类型为 `uint32_t` 的名为 `epoll_event_config` 的成员用于存储当前注册的事件集合. 等到构建 `epoll_event` 类型的对象的时候再现场令该对象的 `data.ptr` 成员指向 `EventDispatcher` 对象自身, 这样就建立起从活跃事件 `active_events` 到 `EventDispatcher` 对象的映射, 以便 `EventPoller` 返回活跃对象 (而不是活跃事件) 的列表.
  - 成员包括:
    - `epoll_event_config`: 当前监听事件集.
    - `active_events`: 活跃事件集, 由 `EventPoller` 进行设置.
    - 各个事件对应的回调.
  - 提供的 API 包括:
    1. 注册处理各种事件的 (无参数且无返回值的) 回调函数.
    1. 设置文件描述符的 epoll 事件监听状态.
    1. 设置活跃事件.
    1. 在设置了活跃事件的前提下对所注册的回调函数关于活跃事件进行分发.
- [ ] `EventPoller`: 对 epoll 机制的抽象.
  - 提供的 API 包括:
    1. 在构造函数中创建 epoll 内核事件表.
    1. 接受一个 `EventDispatcher` 对象并视情况注册或更改其事件监听配置.
    1. 对 `epoll_wait` 进行封装, 返回活跃的 `EventDispatcher` 对象 (的裸指针) 的列表. 之后会由事件循环 `EventLoop` 内部负责遍历这一列表并逐一分发事件.
- [ ] `FileDescriptor`: 对文件描述符的抽象, 作为各个分化的文件描述符类型的公共基类. 其成员仅包含一个文件描述符 `_fd` 和一个 `EventDispatcher` 类型的对象.
- [ ] `Eventfd`: 对 eventfd 的抽象.
- [ ] `Timerfd`: 对 timerfd 的抽象.
- [ ] `EventLoop`: 用于收纳事件循环相关的代码.
  - 需要实现一个谓词用于判断当前线程是否是该 `EventLoop` 所属的线程.
  - 事件循环的逻辑包括:
    1. 事件循环函数主体, 即不断调用 `EventPoller` 对象成员的接口, 并执行各个活跃 `EventDispatcher` 对象的回调.
    1. 在初始化的时候注册一个 eventfd, 通过监听 eventfd 实现异步唤醒, 以便处理外部调用者所注册的回调.
       - 提供接口用于间接向 `FunctorQueue` 对象成员中添加函子 (即 muduo 中实现的 `runInLoop` 和 `queueInLoop`).
    1. 在初始化的时候注册一个 timerfd, 通过监听 timerfd 实现定时机制, 以便处理外部调用者所注册的回调.
       - 提供接口用于间接向 `TimerContainer` 对象成员中添加定时器 (即 muduo 中实现的 `runAt` 和 `runAfter`).
- [ ] `EventLoopThread`: 对事件循环工作线程的抽象. 内部封装一个 `Thread` 对象, 同时实现一个包装函数用于在新线程中初始化并启动一个 `EventLoop` 对象.
- [ ] `EventLoopThreadPool`: 对事件循环工作线程池的抽象. 提供接口用于随机抽取一个工作线程.
- [ ] `MutableSizeTcpBuffer`: TCP 连接专用的变长缓冲区, 用于保存从一个套接字描述符中读取的或是将要向一个套接字描述符中写入的数据.
- [ ] `TcpConnectSocket`: 对 TCP 连接套接字的抽象. 内含两个 `MutableSizeTcpBuffer` 缓冲区, 一个用于接收, 一个用于发送.
- [ ] `InetAddress`: 对套接字地址的抽象. 为 IPv4 和 IPv6 地址提供统一的接口.
- [ ] `ListenSocket`: 对监听套接字的抽象.
- [ ] `TcpServer`: 对 TCP 服务器的抽象. 执行的逻辑包括创建 `ListenSocket` 对象并设置回调, 创建并启动线程池, 以及启动事件循环等. 内部使用一个 map 来索引已建立的 `TcpConnectSocket` 连接.
- [ ] `TcpClient`: 对 TCP 客户端的抽象. 与服务器不同, 客户端只需在主线程维护一个 TCP 连接即可.
- [ ] `Signalfd`: 对 signalfd 的抽象.
- [ ] `HttpContext`: 用于在不连续的数据接收事件之间维护一个逻辑上连续的解析过程. 本对象是存储在 `TcpConnectSocket` 对象中的 (muduo 实现的 `TcpConnectSocket` 对象中设置了一个通用的上下文对象成员 `boost::any context_` 用于存储任意上下文, 其中自然也包括 HTTP 上下文 `HttpContext`).
- [ ] `HttpRequest`: 对 HTTP 请求报文的抽象, 提供一些基础的 API, 包括获取请求头信息等等.
- [ ] `HttpResponse`: 对 HTTP 响应报文的抽象, 提供一些基础的 API, 包括设置响应体数据等等.
- [ ] (基础) 实现一个 echo 服务器.
- [ ] (进阶) 以 long polling 方式开发一个简单的实时聊天室应用.

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
1. `Timestamp`: 对时间戳的抽象, 内部使用一个 `int64_t` 类型的变量表示自 epoch 以来的毫秒数 (一年约有 $2^{44.84}$ 毫秒).
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
