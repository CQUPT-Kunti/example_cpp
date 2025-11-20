
#include "time.h"
#include "TimerManager.h"
#include <iostream>

int main()
{
    TimerManager timerManager;

    Timer fun1([]()
               { std::cout << "Timer callback executed!" << std::endl; }, 2000, &timerManager);

    Timer fun2([]()
               { std::cout << "Timer callback ABC!" << std::endl; }, 4000, &timerManager);
    fun1.start();
    fun2.start();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    timerManager.stop();
    return 0;
}
