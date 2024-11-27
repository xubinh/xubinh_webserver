#ifndef XUBINH_SERVER_UTIL_THREAD
#define XUBINH_SERVER_UTIL_THREAD

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unistd.h>

namespace xubinh_server {

namespace util {

class Thread {
public:
    using WorkerFunctionType = std::function<void()>;

private:
    bool _is_started = 0;
    bool _is_joined = 0;
    int _join_result = 0;
    pthread_t _pthread_id = std::numeric_limits<pthread_t>::max();
    pid_t _tid = -1;
    WorkerFunctionType _worker_function;
    const std::string _thread_name;

    // 为外部线程竞争该线程对象的初始化操作提供保护
    std::mutex _mutex_for_thread_state;

    // 在执行初始化的线程和该线程对象所封装的线程 (其负责初始化线程的状态)
    // 之间提供同步机制
    std::mutex _mutex_for_thread_info;
    std::condition_variable _cond_for_thread_info;

    void _wrapper_of_worker_function();

    void _do_start(std::unique_lock<std::mutex> lock);

    void _do_join(std::unique_lock<std::mutex> lock);

    static void *_adaptor_function_for_pthread_create(void *arg);

public:
    // 每个线程都应该从头开始维护自己的上下文, 因此不允许拷贝
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    // 虽然可以移动但没必要
    Thread(Thread &&) = delete;
    Thread &operator=(Thread &&) = delete;

    // 禁用默认构造函数
    Thread(WorkerFunctionType worker_function, const std::string &thread_name);

    bool is_started() const {
        return _is_started;
    }

    bool is_joined() const {
        return _is_joined;
    }

    void start();

    pid_t get_tid();

    int join();
};

} // namespace util

} // namespace xubinh_server

#endif