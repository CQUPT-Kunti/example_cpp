#include <atomic>
#include <mutex>
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <condition_variable>
#include <future>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool();

    // 加入任务
    template <typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::queue<std::function<void()>> tasks; // 任务队列
    std::vector<std::thread> workers;        // 工作线程池

    std::atomic_bool stop;
    std::mutex mutex;
    std::condition_variable condition;
};
