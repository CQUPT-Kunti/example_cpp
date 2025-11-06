#pragma once

#include "tokenManage.h"

#include <thread>
#include <atomic>
#include <functional>
class consume
{
private:
    tokenManage &manage;
    std::thread consume_thread;
    std::atomic<bool> consume_run;
    std::function<void(bool)> CallBack_;
    int consume_num_everytime;
    int consume_num;

    std::chrono::steady_clock::time_point statr_time;
    std::chrono::steady_clock::time_point end_time;

public:
    consume(tokenManage &manage_, int everytime, int consumeNum, std::function<void(bool)> call_back = nullptr);
    ~consume();
    void start();
    void stop();
    long long time_computer();
};