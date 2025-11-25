#include "Timer.h"

Timer::Timer(int timeout_ms_, func func_, bool repeat_)
    : timeOut_ms(timeout_ms_), callBackFunc(func_), repeat(repeat_)
{
    reset_expire();
}

void Timer::reset_expire()
{
    expire = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeOut_ms);
}
