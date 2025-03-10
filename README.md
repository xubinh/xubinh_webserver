# xubinh's webserver

## 目录

- [部署本项目](#%E9%83%A8%E7%BD%B2%E6%9C%AC%E9%A1%B9%E7%9B%AE)
  - [HTTP 服务器](#http-%E6%9C%8D%E5%8A%A1%E5%99%A8)
  - [echo 服务器](#echo-%E6%9C%8D%E5%8A%A1%E5%99%A8)
- [基准测试](#%E5%9F%BA%E5%87%86%E6%B5%8B%E8%AF%95)
  - [HTTP 服务器](#http-%E6%9C%8D%E5%8A%A1%E5%99%A8)
    - [横向比较](#%E6%A8%AA%E5%90%91%E6%AF%94%E8%BE%83)
  - [日志框架](#%E6%97%A5%E5%BF%97%E6%A1%86%E6%9E%B6)
  - [测试机硬件参数](#%E6%B5%8B%E8%AF%95%E6%9C%BA%E7%A1%AC%E4%BB%B6%E5%8F%82%E6%95%B0)
- [项目结构介绍](#%E9%A1%B9%E7%9B%AE%E7%BB%93%E6%9E%84%E4%BB%8B%E7%BB%8D)
- [其他](#%E5%85%B6%E4%BB%96)
  - [WebBench](#webbench)
    - [安装](#%E5%AE%89%E8%A3%85)
    - [使用示例](#%E4%BD%BF%E7%94%A8%E7%A4%BA%E4%BE%8B)
    - [参考资料](#%E5%8F%82%E8%80%83%E8%B5%84%E6%96%99)
  - [muduo 项目中所采用的抽象](#muduo-%E9%A1%B9%E7%9B%AE%E4%B8%AD%E6%89%80%E9%87%87%E7%94%A8%E7%9A%84%E6%8A%BD%E8%B1%A1)
    - [参考资料](#%E5%8F%82%E8%80%83%E8%B5%84%E6%96%99)
- [待办](#%E5%BE%85%E5%8A%9E)

## 部署本项目

### HTTP 服务器

编译并启动 HTTP 服务器:

```bash
./script/build.sh && ./script/http/run_server.sh
```

然后在浏览器中访问 `http://127.0.0.1:8080/` 即可.

### echo 服务器

编译并启动 echo 服务器:

```bash
./script/build.sh && ./script/echo/run_server.sh
```

然后在另一窗口中启动 echo 客户端即可:

```bash
./script/echo/run_client.sh
```

## 基准测试

### HTTP 服务器

测试命令:

```bash
./script/http/benchmark.py 10 15 # 单次测试持续 10 秒, 连续执行 15 次测试
```

> - 注: 由新到旧降序排序.

| 项目改进描述                                                                                                                                                                                  | 短连接 QPS | 长连接 QPS | commit (点击链接可跳转)                                                                                 |
| --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------- | ---------- | ------------------------------------------------------------------------------------------------------- |
| 将 TCP 连接对象的析构工作从主线程转移至单独的工作线程                                                                                                                                         | 51,082     | -          | [`be36b79`](https://github.com/xubinh/xubinh_webserver/commit/be36b790209f679ff0f875d299e49904badb663b) |
| 实现字符串缓冲区的内存分配器, 并将其应用至 TCP 连接与 HTTP 服务器中                                                                                                                           | 53,298     | -          | [`9c797f6`](https://github.com/xubinh/xubinh_webserver/commit/9c797f6ed348811936dcc555dc6ff3fe8e7c116b) |
| (注: 以上为 `-O3` 优化后的测试结果)                                                                                                                                                           |            |            |                                                                                                         |
| 重构所有 static slab allocator, 放弃 "对 non-static allocator 进行包装" 的方案并直接对内存池进行管理以避免函数调用所引入的额外开销                                                            | 48,362     | -          | [`a5003b6`](https://github.com/xubinh/xubinh_webserver/commit/a5003b61fd9f4876991ceac37934b390233df218) |
| 使用 thread local allocator 方案替代前一版本中 ``std::shared_ptr` 所使用的 lock-free allocator 方案                                                                                           | 49,004     | -          | [`c587c90`](https://github.com/xubinh/xubinh_webserver/commit/c587c90d7b956b78da2334155f40f85734dbb028) |
| 使用 `std::map` 作为 TCP 连接对象的容器, 并使用手写的 slab allocator 替代 `std::map` 与 `std::shared_ptr` 默认所使用的 `std::allocator`                                                       | 49,232     | -          | [`74b1617`](https://github.com/xubinh/xubinh_webserver/commit/74b161727979f6db904aa8a24dacf813427b34ee) |
| 使用无锁队列方案 (lock-free queue + `new`) 替换阻塞队列方案 (`std::deque` + `std::mutex`), 在减少同步机制的使用所导致的性能影响的同时将内存分配与容器进行解耦, 为后续优化内存分配提供操作空间 | 50,879     | -          | [`0145b61`](https://github.com/xubinh/xubinh_webserver/commit/0145b61f2254796585907982e50a97958c4fb17c) |
| 将 `EventPoller` 的文件描述符登记容器更改为定长布尔数组, 放弃 `std::unordered_map`                                                                                                            | 51,903     | -          | [`aa544e5`](https://github.com/xubinh/xubinh_webserver/commit/aa544e5c0d8e57372cb8677d0325a51dc8fc785e) |
| 为 `Any` 添加原地初始化方法, 消除不必要的拷贝/移动初始化                                                                                                                                      | 51,791     | -          | [`4c98acb`](https://github.com/xubinh/xubinh_webserver/commit/4c98acb793fecc7224d6b033bd43f665fe16c183) |
| 使用右值引用避免对 `std::function` 的不必要的重复移动                                                                                                                                         | 52,591     | -          | [`bf42f6f`](https://github.com/xubinh/xubinh_webserver/commit/bf42f6f89cdf2857cc25b9e3267ca02b84efbe6a) |
| 删除 `HttpRequest` 的初始化时间戳的不必要的系统调用                                                                                                                                           | 54,888     | -          | [`afc6e38`](https://github.com/xubinh/xubinh_webserver/commit/afc6e38f4c0f1804fdc85c49999d367ac5d8f13b) |
| 将 `EventPoller::poll_for_active_events_of_all_fds` 方法从 "按值返回 `std::vector`" 改为 "按引用传入", 并消除 `TcpServer` 中创建新 TCP 连接对象时对 `std::shared_ptr` 的重复拷贝              | 54,485     | -          | [`0b33da7`](https://github.com/xubinh/xubinh_webserver/commit/0b33da78ac47c6301b4e256ee432fdfcf1808d2f) |
| 降低 `EventLoop` 的 timerfd 和 eventfd 的系统调用的频率                                                                                                                                       | 51,750     | -          | [`85855f8`](https://github.com/xubinh/xubinh_webserver/commit/85855f85c9336a18411e0d44010b4a804963e936) |
| 降低 TCP 连接的时间戳初始化的 `clock_gettime` 系统调用的执行粒度                                                                                                                              | 49,534     | -          | [`2efc904`](https://github.com/xubinh/xubinh_webserver/commit/2efc904c2e35509707b320cbcea01dc7f5dd0611) |
| 使用 lambda 表达式替换绝大多数的 `std::bind`                                                                                                                                                  | 45,970     | -          | [`6b8a854`](https://github.com/xubinh/xubinh_webserver/commit/6b8a85437a6461cf759066222af6d4bd30989b9e) |
| 降低缓冲区的扩展大小, 避免 HTTP 请求体简短但离散的的情况下发生的无意义的内存重分配                                                                                                            | 43,958     | 90,321     | [`1401078`](https://github.com/xubinh/xubinh_webserver/commit/14010785ea5ea7f38f8848ac0776d5d0ddb1caa5) |
| 将 TCP 连接的单独的 non-blocking 设置操作整合至 `accept4` 调用中                                                                                                                              | 42,302     | 92,049     | [`0f5cf40`](https://github.com/xubinh/xubinh_webserver/commit/0f5cf40b5ed1e6a0fde23f3017e657aa2419046f) |
| 将 `HttpRequest` 恢复为可复制的, 并取消 `HttpParser` 中的 `shared_ptr`                                                                                                                        | 39,577     | 96,732     | [`e823334`](https://github.com/xubinh/xubinh_webserver/commit/e823334b7b9a944dcc9a179d2c43e7bd2c46cfac) |
| 将 TCP 连接对象的容器从 RBT 改为 Hash Table                                                                                                                                                   | 41,323     | 92,449     | [`60554e9`](https://github.com/xubinh/xubinh_webserver/commit/60554e960918c790de1fcd1c26864dffdc84f085) |
| 取消 `TcpServer::_close_callback` 中对 `shared_ptr` 的值捕获                                                                                                                                  | 40,023     | 90,434     | [`6f1c4c8`](https://github.com/xubinh/xubinh_webserver/commit/6f1c4c8b9b9e928d1376ad660bdfa77d96fb891f) |
| 为每个工作线程在主线程中独立配备阻塞队列                                                                                                                                                      | 37,852     | 80,460     | [`c48a407`](https://github.com/xubinh/xubinh_webserver/commit/c48a4075680bf022096cc6e4103ac98512d669dd) |
| 初代稳定版本                                                                                                                                                                                  | 38,661     | 84,392     | [`2794336`](https://github.com/xubinh/xubinh_webserver/commit/2794336a6d619f14d15ef84f438e6b60ec934310) |

#### 横向比较

| 项目名称                                                   | 短连接 QPS | 长连接 QPS | commit                                                                                           |
| ---------------------------------------------------------- | ---------- | ---------- | ------------------------------------------------------------------------------------------------ |
| [linyacool/WebServer](https://github.com/xubinh/WebServer) | 36,062     | 86,438     | [`a50d635`](https://github.com/xubinh/WebServer/commit/a50d635f48178c89f78b4be9d2579613b2c7debf) |

### 日志框架

测试命令:

```bash
./benchmark/logging/run.sh
```

测试条件:

- 在配置相同的前提下写入 1000000 条日志.
- 仅执行单次测试作为最终结果.

| 框架名称           | 用时        | 平均写入速率  |
| ------------------ | ----------- | ------------- |
| spdlog             | 0.260904 秒 | 3832833 条/秒 |
| xubinh log builder | 0.177555 秒 | 5632055 条/秒 |

### 测试机硬件参数

> - 注: 所有测试均在单机环境下完成.

```text
$ sudo lshw -short
H/W path    Device    Class      Description
============================================
                      system     Computer
/0/0                  memory     16GiB System memory
/0/1                  processor  Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz
/0/7/0.0.0  /dev/sda  volume     388MiB Virtual Disk
/0/7/0.0.1  /dev/sdb  volume     4GiB Virtual Disk
/0/7/0.0.2  /dev/sdc  volume     256GiB Virtual Disk
```

## 项目结构介绍

### `include/`

#### `event_loop.h`

- 定义了 event loop 类, 用于对事件循环进行抽象. 每个 event loop 封装了一个 event poller, 一个专门为线程间传递 functor 而特化的 blocking queue (或 lock-free queue, 可通过编译选项进行自主选择) 的数组, 与这些 functor queue 配套的 eventfd (其中一个 eventfd 对应一个 functor queue 以降低并发竞争的程度), 一个 timer container, 以及与其配套的 timerfd.
- event loop 类所封装的最简单但也是最重要的方法是 `.loop()` 方法, 该方法的大意是使用一个无限循环不断轮询 event poller 并获取 event dispatcher, 调用每个 event dispatcher 的回调以分发事件, 然后检查 eventfd 和 timerfd 并调用它们各自的回调.
- 使用多个 queue 的理由是如果主线程的 event loop 只使用一个 queue 作为外部所有工作线程的交流媒介, 那么这个 queue 可能成为性能的瓶颈 (在本项目中不明显, 但在大规模并发场景下可能发生). 为了能够使主线程的 event loop 能够分别为每个工作线程维护一个 functor queue, 这里直接将 event loop 的 functor queue 从根本上设计为了数量可拓展的, 于是主线程可根据工作线程的数量自由选择配套的 functor queue 的数量, 而工作线程则仍然使用默认的单个 functor queue.
- 为了进一步降低并发竞争程度, 每个 eventfd 使用了一个配套的 atomic 标志位来表示其是否被触发, 只有在确认没有被触发时才会执行 eventfd 的系统调用; 另一方面 timerfd 也只会在本次更新能够将定时器的触发时间点提前到一定阈值时 (例如提早 3 秒) 才会执行 timerfd 的系统调用.

#### `event_loop_thread.h`

- event loop thread 类的主要作用是作为从 event loop 类到 thread 类的适配器, 将 event loop 类的 `.loop()` 方法适配为 thread 类能够执行的通用的无参数无返回值的 worker function, 同时也能作为对专门执行事件循环的 (工作) 线程的抽象并为外界提供一个简单且统一的接口.
- 成员函数 `._worker_function()` 的作用便是对 event loop 类的 `.loop()` 方法进行适配, 其大意是在线程栈帧中创建一个 event loop 对象, 通知主线程该对象已经创建, 然后调用该 event loop 对象的 `.loop()` 方法.
- 成员函数 `.start()` 的大意是主线程创建并启动工作线程, 然后进入睡眠并等待工作线程创建 event loop 对象, 等到工作线程创建好 event loop 对象之后便会唤醒主线程.
  - 之所以主线程需要等待工作线程是因为 event loop 对象是在工作线程的栈帧中创建的, 而为了确保工作线程状态的原子性主线程又必须等待 event loop 对象创建好才能继续执行, 因此主线程必须等待工作线程的信号. 这是通过设置一个对应的信号量来做到的.

#### `event_loop_thread_pool.h`

- event loop thread pool 类的主要作用是对线程池进行抽象. 内部使用了一个 `std::vector` 来管理多个 event loop thread 的 `std::shared_ptr`.
- 线程池选择下一个线程的算法使用的是 Round-robin scheduling, 等价于将所有线程排成环然后按顺时针依次选取每一个线程. Round-robin 算法的优点是每个线程的任务的个数平均, 而缺点也是平均, 因为有可能一个任务就耗尽了一个线程的 CPU 时间从而导致其他分配到该线程的任务饥饿, 但在 TCP 服务器的情况下我们可以期望每个 TCP 连接的任务的负载基本上是相同的, 此时 Round-robin 算法是最为合适的.
- 线程池对象的构造函数中并不创建线程, 而是推迟到 `.start()` 方法中再进行创建, 这期间允许用户传入一些自定义的线程初始化函数等等.
- 线程池的停止遵循两步原则, 首先是通过 `.stop()` 方法通知各个工作线程的 event loop 尽快停止执行并跳出循环, 然后通过 `.is_joinable()` 方法轮询线程池中的各个线程是否能够 join 并在确认能够 join 之后再执行 join.
  - 之所以要将线程池的停止分解为 stop 和 join 两步是因为本项目的 event loop 需要支持 elegant shutdown, 为了能够在 shutdown 之前处理完所有待处理的 TCP 连接, event loop 仍然有可能 emit 出来一些 functor 至主线程的阻塞队列中, 如果主线程在 stop 之后立即执行 join, 就有可能因为工作线程等待主线程的阻塞队列空出位置并且主线程等待工作线程因而无法将阻塞队列空出位置而导致死锁.

#### `event_poller.h`

- event poller 类的主要作用是对 epoll 系列的系统调用进行封装, 其中构造函数负责创建 epoll fd, `.register_event_for_fd()` 方法负责调用 `epoll_ctl()` 来为指定的普通 fd 注册事件, `.poll_for_active_events_of_all_fds()` 方法负责调用 `epoll_wait()` 来监听活跃事件.

#### `eventfd.h`

- eventfd 类的主要作用是封装 eventfd 系列的系统调用, 其中 `.increment_by_value()` 方法负责向 eventfd 中递增指定的整数值, `._retrieve_the_sum()` 方法负责从 eventfd 中读取整数值.

#### `inet_address.h`

- inet address 类主要是对 `sockaddr_xxx` 系列的数据结构以及一些相关的系统调用进行封装.
- 每个 inet address 类的对象在创建之后即为有效, 也就是说不允许先创建一个临时的 inet address 对象然后填入相关信息. 这是通过强制删除默认构造函数来做到的. 这么做的理由是 inet address 对象支持对内部的 socket 地址的信息进行查询, 例如是否为 IPv4, 端口号是多少等等, 而强制 inet address 类的对象在创建之后有效则能够确保用户在查询信息时内部的 socket 地址总是有效的, 从而降低思维负担.

#### `listen_socketfd.h`

- listen socketfd 类的作用是对 listen fd 进行抽象, 同时收纳并封装一些与 listen fd 有关的系统调用.
- listen socketfd 对象默认是 level-triggered 且 non-blocking 的. 之所以不选择 edge-triggered 是因为当系统当前打开的文件描述符达到上限时需要跳出循环并前去关闭已经停止但仍然空占文件描述符的 TCP 连接, 但此时监听队列中可能仍然存在已经建立的 TCP 连接未被读取, 这与 edge-triggered 模式的原则相悖, 并可能导致一种 "客户端等待服务器接起连接, 而服务器等待客户端发来新连接以便重新启动循环" 的死锁情况. 另一方面, 之所以不选择 blocking 则是考虑到并发性能问题, 因为如果选择 blocking, 那么我们就无法通过 "尝试接起连接" 这个动作来判断当前是否还有连接, 于是我们就只能一次接起一条连接并通过 level-triggered 模式的特性来判断当前是否还有连接, 这是十分低效的, 通过选择 non-blocking 我们便能够在同一次循环中连续接起多个连接, 提高并发性能.

#### `log_buffer.h`

- log buffer 类提供的是对**定长**字符串缓冲区的抽象. 其中缓冲区的大小通过模板参数在编译期进行指定. 本项目仅仅预定义了两种不同的缓冲区大小, 分别是 log **entry** buffer size (4 KB) 和 log **chunk** buffer size (4 MB), 前者用于单个线程的单条日志字符串的存储, 后者用于收集并存储所有线程发送过来的日志字符串. 两种大小所对应的类型通过模板的显式实例化 (explicit instantiation) 进行定义, 避免代码膨胀.
- log buffer 类内部封装了一个 `char[]` 内置数组类型的成员, 并封装了一系列查询函数以及一个 `.append()` 成员函数来支持字符串的构建.

#### `log_builder.h`

- log builder 类是日志系统向用户开放的接口, 提供了一系列宏 (例如 `LOG_INFO` 等) 以及一系列流式输出运算符 `<<` 的重载来支持日志信息的构建.
- 每个形如 `LOG_INFO` 的宏的内部实际上创建了一个临时的 log builder 对象, 然后通过在该对象上调用重载的 `<<` 运算符来格式化字符串并构建日志信息. 当这个临时的 log builder 对象销毁时, 其内部会自动调用 log collector 类的函数以将其构建好的日志信息发送至后台的日志收集线程处.
- 使用临时对象的好处是存储空间直接在栈上开辟, 无需动态分配内存, 从而能够最大限度提升日志系统的效率, 这也是为什么需要 log buffer 类提供编译期已知的大小的原因.
- 为了进一步加速日志的构建, 在日志信息的格式化方面本类也下足了功夫, 例如定义了编译期函数来获取 `__FILE__` 和 `__FUNCTION__` 等编译期字符串的属性, 通过 time point 类提供的时间戳缓存机制提高时间戳字符串的构建速度, 通过 this thread 命名空间提供的 tid 缓存机制提高 tid 字符串的构建速度, 以及使用哈希表提高指针字符串的构建速度等等.

#### `log_collector.h`

- log collector 类使用单例模式实现了日志的后台收集系统. 其大意是将日志收集系统的逻辑放到一个工作函数 `._background_io_thread_worker_functor()` 中并启动一个 thread 对象来执行, 然后其他线程通过调用 `.get_instance()` 函数来获取单例对象并通过调用 `.take_this_log()` 方法发送日志至后台线程.
- log collector 类在后台线程的栈中创建了一个 log file 类对象来与硬盘文件进行交互并写入日志.
- 日志收集的关键是提高收集速度和尽可能降低慢速的硬盘 I/O 对收集速度的负面影响. 对于提高收集速度, log collector 类使用了主副两个日志 chunk 缓冲区来收集日志, 其中主缓冲区负责主要的收集工作, 而副缓冲区负责在高并发时主缓冲区不够用的情况下进行顶替, 如果副缓冲区也不够则直接在堆上动态分配内存. 另一方面, 对于降低硬盘 I/O 的负面影响, log collector 类主要是通过将日志的收集和写入硬盘分开执行来实现的, 其中工作线程负责调用前台的 `.take_this_log()` 收集日志, 而后台的日志线程则负责将日志写入硬盘, 写入硬盘的逻辑位于 `._background_io_thread_worker_functor()` 函数中.
- log collector 类支持 flush 操作, 这是通过定义一个 atomic 标志位 `._need_flush` 来做到的. 后台的日志线程通过在循环中检查该标志位来判断是否需要 flush.
- log collector 类还支持 abort 操作, 即直接在日志层面对进程执行终止, 后台线程将负责在终止之前将剩余的日志写入文件.
- 对于 log collector 单例对象的生命周期的管理, 本项目采用的方案是使用另一个 clean up helper 类来管理 log collector 类的单例对象的析构. 这么做的原因是有的时候全局对象会早于 log collector 的单例对象进行构造并反过来晚于 log collector 的单例对象进行析构, 而该全局对象的析构函数中有可能还要用到 log collector 的单例对象, 此时单例对象可能已经被析构, 从而导致悬空指针的问题. 为了解决这一问题, log collector 直接手动在堆上对单例对象进行动态分配并将得到的指针交给 clean up helper 类进行析构, 这样便能通过控制 clean up helper 类对象的构造时机来间接 (且精确) 地控制 log collector 单例对象的析构时机.
  - 但这一方法仅限同一个 translation unit 中的情况, 由于不同 translation unit 中的全局对象的初始化顺序并没有被良定义, 因此用户必须确保其他任何 translation unit 中均不存在引用日志线程单例对象的全局变量, 否则本方法将失效. 这一点很容易做到, 毕竟要引用日志线程单例对象就必然要创建日志线程, 而一个程序员本来就应当避免在全局变量中创建线程.
  - 如果仍然考虑局部静态变量的单例模式, 那么用户需要确保任何用到 log collector 单例对象的其他对象在 main 函数中被析构. 这可能还不如一开始手动定义一个 clean up helper 类的对象然后撒手不管来得方便.

#### `log_file.h`

- log file 类用于对一个大小无限的物理文件进行抽象, 其基本思路是在内部维护一个 append only physical file 类型的指针指向当前文件, 并在当前文件的大小超出一定阈值时及时更换文件.
- 更换文件的条件除了文件大小超出阈值以外, 还包括每天一次的强制切换文件.
- 此外除了更换文件, log file 类还会每隔一定的时间间隔将内存中的文件内容 flush 至硬盘中.

#### `pollable_file_descriptor.h`

- pollable file descriptor 类用于对 "可监听事件的文件描述符" 这一概念进行抽象, 在文件描述符 fd 和事件循环 event loop 之间建立起桥梁, 并作为各个具体分化的文件描述符类型的基本构成部分.
- 每个 pollable file descriptor 类的对象通过一组 `.register_xxx_callback()` 方法来允许外部用户将自定义的回调注册至该对象中, 并通过另一组 `.enable_xxx_event()` 方法注册监听事件至 event loop 中. 当 event loop 监听到活跃事件后便会调用 pollable file descriptor 的 `.dispatch_active_events()` 方法对活跃事件进行分发 (即调用各个事件对应的回调).
- 由于各个事件对应的回调函数是外部用户通过注册进行设置的, pollable file descriptor 对象本身无法确保外部用户用于注册回调的对象的生命周期必然长于其本身, 即有可能出现回调函数执行到一半而外部对象却开始析构, 或是外部对象已经析构而导致回调函数访问悬空指针的问题, 为了解决这一问题, pollable file descriptor 类支持外部用户注册一个 `std::weak_ptr` 弱引用来确保生命周期不变量的正确性. 对于对象依赖于 pollable file descriptor 但与其分离的情况, 这样做能够避免执行回调函数, 而对于对象依赖于 pollable file descriptor 并且将其包含的情况 (例如一个启用了 `std::enable_shared_from_this<T>` 的类), 这样做能够阻止外部对象本身早于 pollable file descriptor 对象被析构.

#### `preconnect_socketfd.h`

- preconnect socketfd 类用于对客户端连接服务器的过程进行抽象, 包括连接, 重试, 以及超时退出等等.
- preconnect socketfd 类并不是对文件描述符的抽象, 这是因为 preconnect socketfd 仅负责连接, 连接成功后底层的 socket fd 则需要转移至真正的文件描述符类型的对象中.
- 为了支持重试, preconnect socketfd 内部使用了定时器 timer 来注册重试回调, 并在重试一定次数之后执行超时错误处理回调并退出.

#### `signalfd.h`

- signalfd 类封装了 signalfd 相关的系统调用, 包括信号的屏蔽与信号的监听等等. 此外还定义了一个帮手类 signal set 来简化关于信号集合的操作.
- 使用 signalfd 类的基本流程为: 首先在主线程中 (且在创建任何线程之前) 屏蔽所有信号, 然后初始化 signalfd 对象并注册用户自定义的信号处理函数. 由于信号在主线程中被屏蔽, 任何线程通过继承主线程的信号掩码同样屏蔽了信号, 因此无需关心对信号的处理, 而主线程通过读取 signalfd 来获取由于被屏蔽而转入 pending 队列中的所有信号并进行分发. 由于用户注册给 signalfd 的回调可能涉及到对 signalfd 自身的停止, 因此 signalfd 有可能需要在信号处理函数之前进行定义, 并在信号屏蔽之后再进行初始化 (因为 signalfd 内部有可能创建日志线程), 这可以通过定义一个 `std::unique_ptr` 并惰性构造来做到.
- 由于 signalfd 的析构函数有可能也使用了日志线程, 因此日志线程的 clean up 对象必须放在 signalfd 的 `st::unique_ptr` 之前进行初始化.

#### `socketfd.h`

- socketfd 类实际上就是一个归纳了若干个 socket 相关的系统调用的 struct 空结构体.

#### `tcp_buffer.h`

- tcp buffer 类用于对变长的字符串缓冲区进行抽象, 其内部使用了 `std::string` 作为默认容器, 并通过直接对底层的指针进行操作来最大化缓冲区的性能.

#### `tcp_client.h`

- tcp client 类用于对**单条**客户端 TCP 连接进行抽象. 其中首先使用 preconnect socketfd 与服务器进行连接, 然后在连接成功之后将底层的 socket fd 转交给 tcp connect socketfd 来进行处理.

#### `tcp_connect_socketfd.h`

- tcp connect socketfd 类用于对 TCP 连接进行抽象, 通过精心设计 TCP 连接状态的转移来确保连接的正确性与稳定性. 此外还支持用户注册一个自定义的上下文对象来保持事务在多个离散的事件之间的逻辑上的连续性.

#### `tcp_server.h`

- tcp server 类用于对服务器进行抽象, 支持高并发场景下的连接建立与释放. 其大意是使用 listen socketfd 来建立客户端 TCP 连接, 使用一个 `std::map` 来存储并索引 TCP 连接, 并维护一个线程池来将 TCP 连接的实际工作转移至工作线程.
- 为了降低高并发情况下动态分配 TCP 连接内存所带来的消耗, tcp server 类使用了 simple slab allocator 类来管理 TCO 连接的内存分配.
- 此外 tcp server 类还支持将 TCP 连接的析构工作转移至后台线程进行, 主线程只需负责接起连接, 从而提高并发效率.

#### `timer.h`

- timer 类负责对单个定时器进行抽象. 一个定时器包括一个过期时间 expiration time point, 一个重复间隔 repetition time interval, 一个间隔次数 number of repetitions left, 以及一个回调函数 callback.

#### `timer_container.h`

- timer container 类负责对定时器容器进行抽象. 容器默认使用 `std::set` 来对定时器进行存储, 其中每个定时器的内存是动态分配的, 容器仅存储定时器的指针. 而之所以选择 `std::set` 是因为多个定时器有可能具有相同的时间戳, 使用 `std::set` 方便同时关于时间戳以及定时器的指针建立全序, 方便定时器的查找.

#### `timer_identifier.h`

- timer identifier 类用于对 "定时器的唯一 token" 这一概念进行抽象, 其中每个 timer identifier 的内部包含一个定时器 timer 的指针, 用户通过 timer identifier 类的对象来标识某个定时器, 从而避免将定时器的指针暴露给用户.

#### `timerfd.h`

- timerfd 类用于对 timerfd 相关的系统调用进行封装, 类似于 signalfd 与 eventfd.

#### `util/`

##### `address_of.h`

- 定义了通用的 `address_of` 函数, 用于获取对象的地址. 之所以需要该函数是因为有的类型可能重载了取值运算符 `&`, 无法直接获取地址.

##### `alignment.h`

- 封装了内存对齐相关的一系列帮手函数, 其中最重要的是 `aalloc` 函数, 用于分配对齐的内存空间.

##### `any.h`

- any 类使用类型擦除技巧来为用户提供注册任意类型的上下文对象的功能. 模仿自 boost::any, 并以此为基础做了一些修改 (主要是优化了使用手法, 例如改为使用 `any_cast<T *>(t_ptr)` 而不是 `any_cast<T>(t_ptr)`).
- 注: any 类仅经过本项目的示例程序的测试, 未经过全面测试.

##### `blocking_queue.h`

- blocking queue 类用于对大小有限的阻塞队列进行抽象, 其中对象的存储可以选择按值存储或按指针存储, 这可以通过设置编译选项来进行控制.

##### `condition_variable.h`

- condition variable 类封装了 pthread 库的条件变量相关的 API.

##### `datetime.h`

- datetime 类封装了以毫秒为单位的时间戳相关的 API.

##### `errno.h`

- 定义了 `strerror_tl` 函数, 用于以 thread local 的方式获取 errno 的字符串表示, 确保线程安全.

##### `format.h`

- 定义了 format 类用于收纳一系列与编译期字符串格式化相关的函数, 主要用于日志系统中.

##### `lock_free_queue.h`

- 定义了 lock free queue, 采用最简单的单生产者单消费者 (single-producer, single-consumer, SPSC) 的形式 (多生产者多消费者的变种即为 Michael & Scott queue), 支持按值形式和按指针形式存储对象.

##### `mutex.h`

- mutex 类封装了 pthread 库的互斥锁相关的 API.

##### `mutex_guard.h`

- 定义了 mutex guard 类, 用于为 mutex 类提供 RAII 语义.

##### `physical_file.h`

- physical file 类用于对单个物理文件进行抽象, 使用了用户空间内存缓冲区来批量化文件读写以加速文件 I/O, 支持 flush 操作.

##### `slab_allocator.h`

- 定义了一系列 slab allocator, 包括:
  - simple slab allocator: 非静态 (即每个对象均维护一个独立的内存池) 单线程内存池. 内部使用简单的链表形式组织空闲 slab.
    - 应用于 TCP server 的用于管理 TCP 连接的 `std::map` 中.
  - semi lock-free slab allocator: 非静态半无锁多线程内存池. 内部使用无锁栈组织空闲 slab, 同时简单使用互斥锁保护 memory chunk 的分配. 此外还使用了计数器以解决 ABA 问题, 并使用了缓存对齐以解决伪共享问题.
  - static simple slab allocator: 静态 (即在类静态成员中维护内存池) 单线程内存池. 内部同样使用简单的链表形式组织空闲 slab, 但为了能够对已分配的 memory chunk 进行释放还额外定义了类静态的帮手类 chunk manager 来管理 memory chunk.
  - static semi lock-free slab allocator: 静态半无锁多线程内存池. 实际上就是半无锁 + 静态二者结合的产物.
    - 应用于 TCP 对象的 `std::shared_ptr` 的 inplace 内存分配中.
  - static thread local slab allocator: 静态 thread local 多线程内存池. 由于无锁栈仍然无法避免多个线程关于同一个内存池的竞争性, thread local 内存池将内存池以 thread local 变量的形式进行维护, 每个线程拥有自己本地独立的内存池, 仅在必要的时候才会通过一个互斥锁访问一个所有线程共享的中心内存池 (例如其他线程的 slab 在本线程进行释放从而使得本线程的空闲 slab 积累过多的时候或是本线程的 slab 在其他线程进行释放从而导致本线程的 slab 泄漏过多的时候).
    - 应用于 TCP 对象的 `std::shared_ptr` 的 inplace 内存分配中.
  - static simple thread local string slab allocator: 静态 thread local 多线程内存池. 每个线程具有自己独立的内存池, 并且内存池中按 2 的幂维护不同大小的空闲 slab 链表. 本类并没有实现线程间的空闲 slab 共享机制 (即中心内存池), 这是因为本类的使用场景一般满足 "本线程分配本线程释放" 的性质, 不存在线程间的 reclaiming 的需求.
    - 应用于 HTTP request, HTTP response, 以及 TCP buffer 中.
- 此外为了能够使最后一个 static simple thread local string slab allocator 用于标准库的 `std::basic_string`, 本文件还定义了一系列适配器函数, 例如 `std::to_string()`, `std::hash` 等等.

##### `this_thread.h`

- 定义了 tid 相关的函数, 以及 thread name 相关的函数. 主要用于日志和 profiling 工具中.

##### `thread.h`

- thread 类用于对线程进行抽象, 其中用户负责传入一个无参数无返回值的回调, thread 类负责其他杂务 (例如设置 tid 与 thread name 等等) 以及针对 pthread 库的 API 进行适配等等.
- 当用户的回调函数成功退出后, thread 对象就能够被正常 join 了, 因此 thread 类的包装函数将负责设置一个标志位 `_is_joinable` 以通知外界本对象已经能够被正常 join. 用户既可以通过设置他们自己的标志位来自主控制 thread 对象的 join 时机, 也可以通过轮询 `_is_joinable` 标志位来被动查询 thread 对象是否已经可以被 join.

##### `time_point.h`

- 定义了 time point 类和 time interval 类分别对 "时间点" 以及 "时间区间" 这两个概念进行抽象, 二者的精度均为纳秒. 其中时间点 time point 类支持向字符串的转换.

##### `type_name.h`

- 封装了 type name demangling 相关的函数.

##### `type_traits.h`

- 定义了 type traits 相关的工具.

## 其他

### WebBench

#### 安装

```bash
git clone https://github.com/EZLippi/WebBench.git
cd WebBench
sudo apt-get install rpcbind libtirpc-dev # 此为依赖项
sudo apt-get install exuberant-ctags # 此为依赖项
# make # 执行前请先查看下方提示
# sudo make install PREFIX=your_path_to_webbench # 可选
```

> [!IMPORTANT]
>
> - 安装之后需要在 MakeFile 中的第 1 行 `CFLAGS` 中添加包含路径 `-I/usr/include/tirpc`, 然后在第 3 行 `OFLAGS` 中添加链接选项 `-ltirpc`.
> - 在 sub-shell 中执行 webbench 时会出现无限重复的 Request 输出, 原因是 sub-shell 的 stdout 默认为 block-buffered, 导致在 fork 时缓冲区中还留存有一定数据并在此后被复制到每个子进程中. 解决办法是在 `fork()` 前添加一行 `fflush(stdout);` 清空缓冲区.

#### 使用示例

```bash
# 短连接 (默认)
./webbench -t 60 -c 1000 -2 --get http://127.0.0.1:8080/ # 持续测试 60 秒, 使用 1000 个并发客户端进程, 使用 HTTP/1.1 协议, 使用 GET 请求, 目标 URL 为 http://127.0.0.1:8080/

# 长连接
./webbench -t 60 -c 1000 -k -2 --get http://127.0.0.1:8080/ # 请参考 [linyacool/WebBench](https://github.com/linyacool/WebBench)
```

> [!TIP]
>
> - 经测试, 上述 60 秒的测试时间过长, 非常容易受到操作系统临时 CPU 占用的影响. 一种更好的方案是 "短时多测", 例如 "单次测试 10 秒, 连续测试 10 次, 并以最大值作为结果".

#### 参考资料

- [linyacool/WebServer](https://github.com/linyacool/WebServer)
- [linyacool/WebBench](https://github.com/linyacool/WebBench)
- [EZLippi/WebBench](https://github.com/EZLippi/WebBench)
- [c - GNU Make in Ubuntu giving fatal error: rpc/types.h: No such file or directory - Stack Overflow](https://stackoverflow.com/questions/78944074/gnu-make-in-ubuntu-giving-fatal-error-rpc-types-h-no-such-file-or-directory)

### muduo 项目中所采用的抽象

> - **由于下列类型是通过与 ChatGPT 的问答生成的, 因此可能并不与实际在 muduo 中使用的类型的名称完全一致**.
> - **以下内容仅为个人理解**.

1. `MutexLock`: 对互斥锁的抽象. C++11 下的等价设施: `std::mutex`
1. `MutexLockGuard`: 对互斥锁的 RAII 机制的抽象. C++11 下的等价设施: `std::lock_guard<std::mutex>`
1. `Condition`: 对信号量机制的抽象. C++11 下的等价设施: `std::condition_variable`
1. `Atomic`: 对原子操作的抽象 (但是并不提供内存顺序控制). C++11 下的等价设施: `std::atomic<T>`
1. `BlockingQueue`: 对 (无大小限制的) 阻塞队列的抽象.
1. `BoundedBlockingQueue`: 对 (固定大小的) 阻塞队列的抽象.
1. `AppendFile`: [TODO]
1. `LogFile`: [TODO]
1. `AsyncLogging`: [TODO]
1. `LogStream`: [TODO]
1. `SourceFile`: [TODO]
1. `Impl`: [TODO]
1. `Logger`: [TODO]
1. `StringPiece`: [TODO]
1. `Buffer`: 管理动态缓冲区, 用于存储 I/O 操作期间 (例如读取客户端发送过来的 HTTP 请求时) 的数据, 优化读写性能.
1. `Socket`: [TODO]
1. `Channel`: 对 socket 文件描述符的抽象, 其中包含事件到来时需要执行的回调函数 (由外部的 Acceptor 或 TcpConnection 类进行注册) 等等.
1. `Acceptor`: 对 listen socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `Connector`: [TODO]
1. `TcpConnection`: 对 connect socket 文件描述符的抽象. 是 `Channel` 类的一个包装类.
1. `Poller`: 对 epoll 机制的抽象.
1. `Timestamp`: 对时间戳的抽象, 内部使用一个 `int64_t` 类型的变量表示自 epoch 以来的毫秒数 (一年约有 $2^{44.84}$ 毫秒).
1. `TimerQueue`: 对计时器容器的抽象.
1. `Timer`: 对单个计时器元素的抽象.
1. `EventLoop`: 对事件循环的抽象. 每个循环中不仅需要处理 epoll 事件的监听, 还要处理定时器和外部注册到当前线程中的任务等等.
1. `ThreadLocal`: 帮手类, 用于实现 C++11 以前的线程局部存储 (TLS) 机制. C++11 下的等价设施: `thread_local` 关键字
1. `CurrentThread`: 使用 `__thread` 关键字存储一些线程独立的信息, 包括对 TID, TID 的字符串形式, TID 字符串的长度, 以及线程名称字符串的缓存. 同时提供了一些 API 用于初始化以及获取这些信息.
1. `ThreadData`: 帮手类, 用于在启动 `Thread` 对象时在启动 `Thread` 对象的线程 (主线程) 和该对象底层所封装的线程 (工作线程) 之间关于 `Thread` 对象的各个状态成员建立同步关系. 由于状态的改变需要在工作线程中进行, 因此还需要对 `Thread` 对象传进来的可调用对象进一步进行封装, 也就是说封装后的可调用对象会先改变状态, 然后调用内部所封装的原始的可调用对象. `ThreadData` 就是可调用对象的类, 它的成员函数 `runInThread` 和全局作用域中的函数 `startThread` 共同构成了可调用对象的调用运算符.
1. `Thread`: 对线程的抽象, 内部封装了 POSIX pthread API, 并提供一系列额外的简单的 API 用于对底层封装的线程执行各种操作, 包括但不限于 `start`, `stop`, `get_tid` 等等. 使用 `Thread` 最重要的原因是它将底层封装的线程的状态映射到 `Thread` 对象的各个相应成员上, 然后通过一系列 API 方便简洁地获取这些状态信息, 更重要的一点是使用 `Thread` 对象还能够控制线程的生命周期, 实现惰性启动 (如果直接使用 `std::thread` 那么在创建对象的那一刻起底层的线程就已经启动了).
1. `EventLoopThread`: 对线程 (实际上主要是对工作线程) 的抽象. 是 `EventLoop` 类的一个包装类. 初始化时自动启动一个 `EventLoop`.
1. `EventLoopThreadPool`: 对线程池的抽象. 是 `EventLoopThread` 类的一个包装类.
1. `InetAddress`: 对套接字地址 (IP + port) 的抽象.
1. `TcpServer`: 对服务器的抽象.
1. `TcpClient`: [TODO]
1. `HttpRequest`: 对 HTTP 请求报文的抽象.
1. `HttpResponse`: 对 HTTP 响应报文的抽象.
1. `HttpContext`: 对 HTTP 请求解析过程的抽象.

#### 参考资料

- [chenshuo/muduo](https://github.com/chenshuo/muduo)

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
- [x] `FixedSizeLogBuffer`: 定长缓冲区, 用于收集日志.
  - 仅提供基本的数据写入接口.
  - `FixedSizeLogBuffer` 将会定义为模板类, 其中唯一的模板实参用于指定缓冲区的大小.
  - `FixedSizeLogBuffer` 主要用于两个场景:
    1. 在 `LogBuilder` 类对象的内部用于存储正在执行拼接的日志信息片段.
    1. 在 `LogCollector` 类单例对象的内部用于存储各个线程中的 `LogBuilder` 类对象送过来的日志数据.
- [x] `current_thread`: 帮手命名空间, 用于管理当前线程的元数据.
  - 主要原理是利用 `thread_local` 关键字创建线程局部变量来缓存线程的 TID (及其字符串形式) 和线程的名称等数据以便加快日志消息的构建.
- [x] `Thread`: 帮手类, 对线程的抽象.
  - 接受一个由外部调用者传入的通用的 (无参数且无返回值的) 回调并将其存储在内部, 提供 API 实现对线程的惰性启动.
  - 内部封装了底层线程的执行状态以及 TID 等信息, 并提供对应的 API 用于获取这些信息.
  - 内部并不包含任何能够控制底层线程的执行状态的机制, 因此调用者必须将线程状态的同步机制封装在回调中, 并确保在尝试停止线程之前首先利用同步机制通知底层线程, 否则可能导致程序阻塞.
- [x] `LogCollector`: 异步日志收集类, 以单例模式实现.
  - 所有需要使用日志功能的线程均通过这个单例对象调用日志功能.
  - 提供一个 `take_this_log_away` 函数来收集日志数据.
  - 内部预先设置一定数量 (2 个即可, 主要是为了将前端接收与后端写入解耦) 的 `FixedSizeLogBuffer` 对象, 装满的缓冲区构成一个阻塞队列, 并在日志爆满的时候自适应增加新的缓冲区.
  - 任何工作线程在向 `LogCollector` 写入日志前均需要先竞争一个互斥锁.
  - 此外还将创建一个后台线程用于定期收集前端接收到的日志并输出至物理文件中.
  - 需要提供可靠的终止前刷新机制, 确保在进程终止前将错误信息输出至日志中.
- [x] `type_traits`: 帮手命名空间, 用于存放 type trait 相关的基础设施.
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
  - 每个 `LogBuilder` 对象都是一次性的, 其内部初始化一个小型 `FixedSizeLogBuffer`, 并在构建好单条日志数据之后就地销毁. 这样做的好处是将日志构建保持在栈中进行, 而不是使用堆内存的动态请求与释放 (开销较大).
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
- [x] `BlockingQueue`: 定长无超时阻塞队列.
- [x] `FunctorQueue`: `EventLoop` 类专用的阻塞队列, 其元素为无参数且无返回值的函子. 具体实现为 `BlockingQueue` 类的一个显式实例化的实例.
- [x] `TimeInterval`: 高精度时间区间, 精度为纳秒 ($10^{-9}$ 秒). 具体实现为 64 位定长整型 `int64_t` 的别名.
  - 由于一年有 $365 * 24 * 3600 = 31536000 = 2^{24.91}$ 秒, 在精度设置为纳秒的情况下表示一年的纳秒数所需二进制位数也不超过 $25 + 30 = 55$ 位, 放在 `int64_t` 范围内还剩余 8 位, 即 (从 epoch 开始的) 256 年.
- [x] `TimePoint`: 高精度时间点, 基于 `TimeInterval` 实现, 精度为纳秒.
  - 支持与时间区间对象之间的加减运算, 运算结果仍为时间点类型.
  - 支持与时间点对象之间的减法运算, 运算结果为时间区间类型.
  - 支持各种比较运算.
  - 特例化 `std::hash` 类以便支持要求哈希语义的标准库容器, 例如 `std::set` 等.
- [x] `Timer`: 对定时器元素的抽象.
  - 成员包括:
    - 一个代表触发时间的时间点 `TimePoint` 对象.
    - 一个代表重复触发的间隔时间的时间区间 `TimeInterval` 对象. 如果其值为 `0` 则表示一次性定时器.
    - 一个代表重复次数的 `int` 类型对象. 如果其值为 `-1` 则表示无限重复, 如果其值为非负数则表示本次执行之后仍需重复的次数.
    - 一个 (无参数且无返回值的) 回调函数.
  - 提供的 API 包括:
    - 调用一次回调, 并检查该定时器是否仍然有效.
    - 不断调用回调直至该定时器失效或者下一次超时时间严格晚于给定的时间点, 并检查该定时器是否仍然有效.
- [x] `TimerContainer`: 对定时器容器的抽象. 为了效率起见, 内部采用树的实现方式对定时器元素关于超时时间进行排序.
  - 提供的 API:
    - 添加一个定时器.
    - 取消一个定时器.
    - 获取在给定时间点前超时的所有定时器.
  - 还需要提供互斥锁来确保上述 API 的原子性.
  - 关于内部容器采用的实现方式:
    - 一种实现方式是使用堆, 这样做不仅一般操作的时间复杂度较低 (插入操作的时间复杂度为 $O(\log{n})$, 弹出为 $O(1)$), 而且不需要维护树形结构, 只需要一个静态数组即可, 较为节省内存资源. 但其有一个重要缺点是不支持随机删除 (虽然也可以实现, 但是会引入额外的内存消耗并降低执行速度, 抵消掉使用堆带来的好处).
    - 另一种实现方式是使用树, 通过维护有序的树形结构来支持随机删除, 用空间省时间, 其他方面和堆相同.
      - 定时器容器至少需要对定时器关于超时的时间点进行排序. 即使如此, 多个不同的定时器仍然可能在同一个时间点下超时, 因此为了去重还需要将定时器的指针 (或其他标识符, 不过考虑到效率还是裸指针最合适) 纳入索引依据. 但纳入索引依据不代表需要在指针之间建立全序. 从逻辑上讲[指针之间不应该建立全序](https://stackoverflow.com/questions/9086372/how-do-you-check-if-two-pointers-point-to-the-same-object) (这里不是说指针之间无法进行比较, 而是说比较操作本身不具备高层次的语义), 因此应该选择 `std::multimap<K, V>`. 但实际这么做会对执行效率造成影响, 因为必须先找到键, 然后再在这个键下的所有元素中进行顺序遍历. 为了效率考虑, 实际应该选择 (能够同时关于 `K` 和 `V` 执行二分法的) `std::set<std::pair<K, V>>` 作为内部实现 (muduo 中也是这么做的).
- [x] `TimerIdentifier`: 对定时器的裸指针的包装类.
  - 本类为 `EventLoop` 专用.
    - 之所以不是 `TimerContainer` 专用是因为定时器容器需要关于超时时间顺序对定时器元素进行排序, 并通过结合超时时间和定时器元素的指针来唯一索引一个定时器元素, 这决定了定时器容器只能在定时器元素的裸指针上进行操作. 而既然定时器容器位于裸指针的层次, 那么就必然不能够直接和用户进行接触, 而应该通过 `EventLoop` 作为中间者进行间接接触. `EventLoop` 负责接受用户传递进来的回调并生成 `TimerIdentifier`, 同时负责生成裸指针以便对另一边的 `TimerContainer` 进行操作.
- [x] `PollableFileDescriptor`: 对可监听文件描述符的抽象, 作为各个分化的文件描述符类型的公共基类, 为各类可供监听事件的文件描述符提供统一的事件分发的接口.

  - 成员包括:
    - 文件描述符.
    - 当前监听事件集.
    - 活跃事件集.
    - 各个事件对应的回调.
  - 提供的 API 包括:
    1. 注册处理各种事件的 (无参数且无返回值的) 回调函数.
    1. 设置文件描述符的 epoll 事件监听状态.
    1. 设置活跃事件.
    1. 对活跃事件的分发.
  - Linux 下可供监听事件的文件描述符包括:

    - 监听套接字 (listen socket)
    - 连接套接字 (connect socket)
    - 事件描述符 (eventfd)
    - 定时器描述符 (timerfd)
    - 信号描述符 (signalfd)

    框架内部不使用信号描述符, 而是留给用户自己注册想要的回调. 唯一的例外是 `SIGPIPE` 信号, 框架内部必须屏蔽该信号以便后续将其整合到事件循环的处理逻辑中 (即写入错误码 `EPIPE`).

  - epoll 所支持的事件类型:
    - `EPOLLIN`: 通用读事件, 说明有数据待读取. 处理读事件的例子包括 `read()`, `recv()`, 以及 `accept()` 等等.
    - `EPOLLOUT`: 通用写事件, 说明可写入数据. 处理写事件的例子包括 `write()` 和 `send()` 等等.
    - `EPOLLRDHUP`: TCP 套接字专用的关闭事件, 说明对方完全关闭了连接或者关闭了连接的写端. 此时本方仍然能够读取剩余的未读取的数据, 直至遇到 EOF (`read` 操作返回 `0`).
    - `EPOLLPRI`: 通用紧急事件, 说明有紧急数据待读取.
    - `EPOLLERR`: 通用错误事件, 说明文件描述符的内部出现错误. 处理错误事件的例子包括 `getsockopt()` 等等.
      - 此事件无需注册, `epoll_wait` 总是会监听此事件.
    - `EPOLLHUP`: 通用关闭事件, 说明对方完全关闭了连接. 此时本方仍然能够读取剩余的未读取的数据, 直至遇到 EOF (`read` 操作返回 `0`).
      - 此事件无需注册, `epoll_wait` 总是会监听此事件.
    - `EPOLLET`: 标志位, 表示启用 ET 模式.
    - `EPOLLONESHOT`: 标志位, 表示启用 one-shot 模式.
  - 各种事件的处理逻辑:
    - 读事件: `EPOLLIN`, `EPOLLPRI`, `EPOLLRDHUP`.
      - 根据读入字节数确定对方是否关闭了他的写端. 如果对方关闭了他的写端 (即对方主动开始的四次挥手的第一步, 对方向本地发送了 FIN), 那么就关闭读事件的监听, 但此时不能关闭本地的写端, 因为本轮可能还需要读取数据并进行处理和发送, 只有在确定没有数据需要发送的情况下才能关闭本地写端 (即对方主动开始的四次挥手的第三步, 本地向对方发送 FIN). 关闭事件 `EPOLLHUP` 将会在对方发回了 ACK 之后被触发.
    - 写事件: `EPOLLOUT`.
      - 既然启用了写事件的监听就必然说明有数据需要发出. 如果数据在本轮能够发送完毕, 那么就关闭写事件的监听. 如果此时还发现对方关闭了他的写端, 这说明此后也不会再有数据需要发出, 此时完全可以提前关闭本地的写端 (即对方主动开始的四次挥手的第三步, 本地向对方发送 FIN). 关闭事件 `EPOLLHUP` 将会在对方发回了 ACK 之后被触发.
    - 关闭事件: `EPOLLHUP`.
      - 监听到关闭事件说明已经完成四次挥手, 或是连接被异常重置, 不管何种情况均可停止监听所有事件, 但仍然需要检查本轮是否还有数据需要读取.
    - 异常事件: `EPOLLERR`.
      - 发生异常事件并不意味着连接需要立即关闭, 例如当对方异常关闭他的读端的时候, 如果本地继续向对方发送数据, 此时既有可能在写事件中触发 `EPIPE` 错误, 也有可能由于对方发回一个 RST 导致触发本事件. 因此可以选择仅打印错误信息并继续监听事件, 直至关闭事件 `EPOLLHUP` 真正被触发时再行关闭.
  - 由于事件的注册需要用到 `epoll_event` 类型的一个对象, 因此内部需要维护一个类型为 `uint32_t` 的成员用于存储当前注册的事件集合. 等到构建 `epoll_event` 类型的对象的时候再现场令该 `epoll_event` 对象的 `data.ptr` 成员指向 `EventDispatcher` 对象自身, 这样就建立起从 `epoll_wait` 所返回的活跃事件对象到 `EventDispatcher` 对象的映射.
  - 各个派生类的实现规范:
    - 以 `static` 形式包装实现对应类型的文件描述符的创建方法.
    - 降低回调函数注册接口的暴露程度, 仅向外暴露那些对应类型特定的回调函数的注册接口 (例如 TCP 套接字仅应当暴露数据处理接口, 而读事件接口可通过硬编码在内部的方式进行固化).
    - 降低 epoll 事件监听状态更改接口的暴露程度, 只保留那些在该种类型的文件描述符的情况下有意义并且允许用户使用的接口.
    - 包装实现与特定类型的文件描述符相关的 API, 以便用户自由构建回调函数.

- 各个派生类并不负责在构造函数中创建对应类型的文件描述符, 而是将创建时机推迟给用户以便设置一些可能的标志等等. 反之由于文件描述符的使用周期和该文件描述符对象的生命周期相同, 因此各个派生类将统一在析构函数中调用 `::close()` 关闭文件描述符.
- 派生类的用户在构造派生类对象的时候需要做三件事:

  1. 调用派生类专用的函数创建派生类文件描述符, 并初始化对象.
  1. 注册派生类专用的回调函数.
  1. 启用监听派生类专用的事件 (之所以交给用户来做是为了避免事件在回调注册之前就提前触发).

  用户在析构派生类对象的时候不需要做任何事情, 由派生类对象自身的析构函数确保析构之前禁用所有事件的监听并关闭文件描述符.

- [x] `EventPoller`: 对 epoll 机制的抽象.
  - 提供的 API 包括:
    1. 在构造函数中创建 epoll 内核事件表.
    1. 为文件描述符注册事件.
    1. 对 `epoll_wait` 进行封装, 返回活跃的 `EventDispatcher` 对象 (的裸指针) 的列表. 之后会由事件循环 `EventLoop` 内部负责遍历这一列表并逐一分发活跃事件.
- [x] `Eventfd`: 对 eventfd 的抽象. 内部包装了一些 eventfd 相关的基础 API.
- [x] `Timerfd`: 对 timerfd 的抽象. 内部包装了一些 timerfd 相关的基础 API.
- [x] `EventLoop`: 用于收纳事件循环相关的代码.
  - 成员包括:
    - epoll 事件监听器对象.
    - 一个 eventfd 对象.
    - 一个函子专用的阻塞队列.
    - 一个 timerfd 对象.
    - 一个定时器容器对象.
    - 本对象所属的线程的 TID.
  - 提供的 API 包括:
    - 执行事件循环.
    - 帮文件描述符注册事件.
    - 向阻塞队列中追加函子并利用 eventfd 唤醒事件循环.
    - 向定时器容器中添加或删除定时器, 并利用 timerfd 更新闹钟.
  - 需要实现一个谓词用于判断当前线程是否是该 `EventLoop` 所属的线程.
  - 事件循环的逻辑包括:
    1. 事件循环函数主体, 即不断调用 `EventPoller` 对象成员的接口, 并执行各个活跃 `EventDispatcher` 对象的回调.
    1. 在初始化的时候注册一个 eventfd, 通过监听 eventfd 实现异步唤醒, 以便处理外部调用者所注册的回调.
       - 提供接口用于间接向 `FunctorQueue` 对象成员中添加函子 (即 muduo 中实现的 `runInLoop` 和 `queueInLoop`).
    1. 在初始化的时候注册一个 timerfd, 通过监听 timerfd 实现定时机制, 以便处理外部调用者所注册的回调.
       - 提供接口用于间接向 `TimerContainer` 对象成员中添加定时器 (即 muduo 中实现的 `runAt` 和 `runAfter`).
- [x] `EventLoopThread`: 对事件循环工作线程的抽象. 内部封装一个 `Thread` 对象, 同时实现一个包装函数用于在新线程中初始化并启动一个 `EventLoop` 对象.
- [x] `EventLoopThreadPool`: 对事件循环工作线程池的抽象. 提供接口用于随机抽取一个工作线程.
- [x] `InetAddress`: 对套接字地址的抽象. 为 IPv4 和 IPv6 地址提供统一的接口.
  - 一方面需要提供 API 方便用户以一般字符串形式传入套接字地址并转换至专门的类型, 另一方面也要提供 API 方便框架内部按相反的方向进行转换.
  - 内部使用一个 `sockaddr_in` 和 `sockaddr_in6` 构成的 `union` 结构体同时存储不同类型的套接字地址.
- [x] `MutableSizeTcpBuffer`: TCP connect socketfd 专用的变长缓冲区. 其 API 主要提供给框架外部的用户, 在框架内部基本上是作为占位符使用.
  - 内部使用一个 `std::vector<char>` 作为底层缓冲区的实现. 这里利用的仅仅是其所实现的动态内存分配逻辑, 对于内存块中的数据写入情况则需要包装类自己进行管理, 这意味着 `std::vector` 的 `push_back()` 等相关操作将被包装类接管并被取代. 此时整个缓冲区实现具有从上到下三层的结构, 最底下的一层是 `std::vector` 的 capacity, 中间一层是其 size, 最上面一层则是包装类所负责管理的数据的当前读取/写入位置.
  - 由于使用了 `std::vector`, 在写入之前必须先使用 `resize()` 拓宽可写入范围, 如果在拓宽之前越界写入, 那么下一次 `resize()` 时这一范围内的数据就会被默认初始化 (覆盖) 为 `\0` 字节.
  - 由于同时存在读/写, 因此每当空间不足时还需检查缓冲区的头部是否存在空闲空间, 如果存在足够的空闲空间则可以选择将已有的数据块内容向头部平移并在尾部腾出空间. 空间分配应当遵守的原则如下:
    1. 每次重分配之后均将 `size()` 一步到位扩大至与 `capacity()` 相等.
    1. 检查当前写入位置和 `size()` 之间的空闲空间大小. 如果足够则直接写入, 否则接着往下检查.
    1. 检查从 `begin()` 到当前读取位置的空闲空间加上当前写入位置和 `size()` 之间的空闲空间的总大小.如果足够则先平移后写入, 否则接着往下检查.
    1. 直接调用 `resize()` 申请重分配内存块. 由于此举隐式包含了一次内存复制, 为了效率起见, 重分配之后不实施平移.
  - `std::vector<char>` 的初始 capacity 为 `0`, 并且每当大小超出 capacity 时会重新分配一个两倍大小的新内存块. 为了降低这一行为在频繁分配小型缓冲区时造成的性能影响, 可以适当将初始 capacity 调高一些 (例如 1 KB).
- [x] `Any`: 帮手类, 用于存储任意类型的对象. 仿效自 `boost::any`.
  - 需要定义两个内部的帮手类实现类型擦除模式, 这两个类分别是 `HolderBase` 以及 `Holder`.
    - `HolderBase`: 普通类型, 作为类型擦除模式的基类.
      - 提供的 API 包括:
        - 一个虚的析构函数.
        - 一个纯虚的 `type()` 函数. 派生类在内部调用 `typeid` 表达式, 因此返回值为 `std::type_info` 类型.
        - 一个纯虚的 `clone()` 函数. 派生类实现各自的拷贝函数.
    - `Holder<DecayedNoCvrValueType>`: 派生类, 直接继承自 `HolderBase`, 用于存储类型信息.
      - 成员包括:
        - 一个类型为所委托类型的对象.
      - 提供的 API 包括:
        - 对所委托的值的完美转发.
        - 基类的虚函数的实现.
  - 成员包括:
    - `HolderBase` 类型的裸指针, 指向底层对象.
  - 提供的 API 包括:
    - 一个构造函数模板, 使用完美转发将任意类型的对象转发给底层的 `HolderBase` 对象.
    - 拷贝构造函数, 通过调用 `HolderBase` 的虚 `close()` 函数对底层对象进行拷贝.
    - 移动构造函数, 直接移动底层对象的指针.
    - 交换运算.
    - 根据返回值类型的不同, 需要实现两种不同的 `any_cast` 函数用于取出底层对象:
      - 返回值为指针类型的版本的 `any_cast` 接受 `Any` 对象的指针并返回指向底层对象的指针. 此版本根据形参类型的不同又进一步细分为普通指针和常量指针两种具体实现.
      - 返回值为非指针类型的版本的 `any_cast` 接受 `Any` 对象的引用并返回底层对象的引用. 此版本根据形参类型的不同又进一步细分为普通左值引用, 常量左值引用, 以及右值引用三种具体实现.
- [x] `TcpConnectSocketfd`: 对 TCP connect socketfd 的抽象.
- [x] `ListenSocketfd`: 对 listen socketfd 的抽象.
- [x] `PreconnectSocketfd`: 对预连接的 connect socketfd 的抽象.
- [x] `TcpServer`: 对 TCP 服务器的抽象.
- [x] `TcpClient`: 对 TCP 客户端的抽象.
- [x] `HttpRequest`: 对 HTTP 请求报文的抽象.
- [x] `HttpParser`: 对 HTTP 请求解析过程的抽象, 通过在内部维护必要的上下文信息来为离散的数据接收事件维护一个逻辑上连续的解析过程.
- [x] `HttpResponse`: 对 HTTP 响应报文的抽象.
- [x] `Signalfd`: 对 signalfd 的抽象.
- [x] 实现一个 echo 服务器.
- [x] 实现一个简单的 HTTP 服务器, 使用定时器关闭超时请求.
- [x] 使用 WebBench 对框架进行一次短连接测试.
- [x] 在长连接场景下使用 WebBench 对框架进行一次测试.
- [x] 改进时间戳类, 添加高精度的字符串表示.
- [x] 与其他项目进行横向比较.
- [x] 优化服务器, 提高 QPS.
