#include "TimerManager.h"
#include <iostream>
#include <atomic>
#include <iomanip>

std::atomic<int> task_counter{0};

int main()
{
    ThreadPool pool(4); // 4个工作线程
    TimerManager timer_manager(pool);

    std::cout << "Starting timer manager with 4 worker threads..." << std::endl;
    std::cout << "Creating 20 timer tasks..." << std::endl
              << std::endl;

    std::vector<std::shared_ptr<Timer>> timers;

    // 创建10个单次执行的定时器
    for (int i = 0; i < 10; ++i)
    {
        int task_id = i;
        int delay = 100 + i * 100; // 100ms, 200ms, ..., 1000ms

        auto timer = timer_manager.add_timer(delay, [task_id]()
                                             {
            int count = ++task_counter;
            auto tid = std::this_thread::get_id();
            std::cout << "[Task " << std::setw(2) << count << "] "
                      << "Single timer " << task_id 
                      << " executed (delay: " << (100 + task_id * 100) << "ms) "
                      << "[Thread: " << tid << "]" << std::endl; }, false);

        timers.push_back(timer);
    }

    // 创建10个重复执行的定时器
    for (int i = 0; i < 10; ++i)
    {
        int task_id = i + 10;
        int interval = 300 + i * 50; // 300ms, 350ms, ..., 750ms

        auto timer = timer_manager.add_timer(interval, [task_id, interval]()
                                             {
            int count = ++task_counter;
            auto tid = std::this_thread::get_id();
            std::cout << "[Task " << std::setw(2) << count << "] "
                      << "Repeat timer " << task_id 
                      << " executed (interval: " << interval << "ms) "
                      << "[Thread: " << tid << "]" << std::endl; }, true);

        timers.push_back(timer);
    }

    std::cout << "All 20 timers created (10 single + 10 repeat)" << std::endl;
    std::cout << "Running for 3 seconds..." << std::endl
              << std::endl;

    // 运行3秒观察重复定时器
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nStopping all repeat timers..." << std::endl;

    // 停止所有重复定时器（后10个）
    for (size_t i = 10; i < timers.size(); ++i)
    {
        timer_manager.remove_timer(timers[i]);
    }

    std::cout << "Waiting 1 more second for cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\nTotal tasks executed: " << task_counter.load() << std::endl;
    std::cout << "Program finished." << std::endl;

    return 0;
}
