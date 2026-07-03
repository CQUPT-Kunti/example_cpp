#include <atomic>
#include <mutex>
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <condition_variable>

class ThreadPool
{
public:
    explicit ThreadPool(size_t threadCount);
    ~ThreadPool();

    // 加入任务
    void enqueue(std::function<void()> task);

private:
    std::queue<std::function<void()>> tasks; // 任务队列
    std::vector<std::thread> workers;        // 工作线程池

    std::atomic_bool stop;
    std::mutex mutex;
    std::condition_variable condition;
};
