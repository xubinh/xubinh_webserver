# xubinh's webserver

## 目录

- [部署本项目](#%E9%83%A8%E7%BD%B2%E6%9C%AC%E9%A1%B9%E7%9B%AE)
- [基准测试](#%E5%9F%BA%E5%87%86%E6%B5%8B%E8%AF%95)
  - [HTTP 服务器](#http-%E6%9C%8D%E5%8A%A1%E5%99%A8)
    - [横向比较](%E6%A8%AA%E5%90%91%E6%AF%94%E8%BE%83)
  - [日志框架](#%E6%97%A5%E5%BF%97%E6%A1%86%E6%9E%B6)
  - [测试机硬件参数](#%E6%B5%8B%E8%AF%95%E6%9C%BA%E7%A1%AC%E4%BB%B6%E5%8F%82%E6%95%B0)
- [其他](#%E5%85%B6%E4%BB%96)
  - [WebBench](#webbench)
    - [安装](#%E5%AE%89%E8%A3%85)
    - [使用示例](#%E4%BD%BF%E7%94%A8%E7%A4%BA%E4%BE%8B)
    - [参考资料](#%E5%8F%82%E8%80%83%E8%B5%84%E6%96%99)
  - [muduo 项目中所采用的抽象](#muduo-%E9%A1%B9%E7%9B%AE%E4%B8%AD%E6%89%80%E9%87%87%E7%94%A8%E7%9A%84%E6%8A%BD%E8%B1%A1)
    - [参考资料](#%E5%8F%82%E8%80%83%E8%B5%84%E6%96%99)
- [待办](#%E5%BE%85%E5%8A%9E)

## 部署本项目

首先克隆项目仓库至本地:

```bash
git clone https://github.com/xubinh/xubinh_webserver.git
```

进入项目目录, 克隆 WebBench 仓库至本地:

```bash
cd xubinh_webserver
git clone https://github.com/xubinh/WebBench.git
```

编译 WebBench 得到可执行文件:

```bash
cd WebBench
sudo apt-get install rpcbind libtirpc-dev exuberant-ctags
make
cd ..
mkdir bin
cp -t bin/ WebBench/webbench
```

编译 HTTP 服务器并启动服务器:

```bash
RUN_BENCHMARK=off ./script/http/build.sh && ./script/http/run_server.sh
```

在浏览器中访问 `http://127.0.0.1:8080/` 即可.

## 基准测试

### HTTP 服务器

测试命令:

```bash
./script/http/webbench.py 10 15 # 单次测试持续 10 秒, 连续执行 15 次测试
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
