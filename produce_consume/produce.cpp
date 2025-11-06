#include "produce.h"
#include <iostream>

produce::produce(tokenManage &manage) : tokenGroup(manage)
{
    runState = false;
}

void produce::start()
{
    runState = true;
    go = std::thread([this]()
                     {
                         std::cout << "[produce] thread started.\n";
                         while (runState)
                         {
                             tokenGroup.addTaken();
                             std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                         }
                         std::cout << "[produce] thread stopped.\n"; });
}

void produce::stop()
{
    runState = false;
    if (go.joinable())
    {
        go.join();
    }
}
