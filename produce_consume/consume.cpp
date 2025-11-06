#include "consume.h"
#include <iostream>

consume::consume(tokenManage &manage_, int everytime, int consumeNum, std::function<void(bool)> call_back)
    : manage(manage_)
{
    consume_num_everytime = everytime;
    consume_num = consumeNum;
    CallBack_ = call_back;
}

consume::~consume()
{
    stop();
}

void consume::stop()
{
    consume_run = false;
    if (consume_thread.joinable())
    {
        consume_thread.join();
    }
}

void consume::start()
{
    consume_run = true;
    consume_thread = std::thread([this]()
                                 {
                                     std::cout << "[consume] thread started.\n";
                                     statr_time = std::chrono::steady_clock::now();

                                     while (consume_run.load())
                                     {
                                         if (consume_num <= 0)
                                         {
                                             break;
                                         }

                                         bool success = manage.consumetoken(consume_num_everytime);
                                         consume_num--;

                                         if (CallBack_)
                                         {
                                             CallBack_(success);
                                         }

                                         std::this_thread::sleep_for(std::chrono::milliseconds(800));
                                     }

                                     end_time = std::chrono::steady_clock::now();
                                     std::cout << "[consume] thread finished, total time "
                                               << time_computer() << " ms\n"; });
}

long long consume::time_computer()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - statr_time).count();
}
