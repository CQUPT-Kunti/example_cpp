#include "tokenManage.h"
#include <iostream>

tokenManage::tokenManage(int max_) : taken_maxNum(max_) {
    current_takenNum = 0;
}

bool tokenManage::addTaken()
{
    std::lock_guard<std::mutex> lock(mtx);
    if (current_takenNum == taken_maxNum)
    {
        std::cout << "[produce] reach max, stop adding.\n";
        return false;
    }
    current_takenNum += 1;
    std::cout << "[produce] add 1 token, current: " << current_takenNum << std::endl;
    cond_.notify_one(); // 唤醒等待的消费者
    return true;
}

bool tokenManage::tryConsumeToken(int n)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (current_takenNum - n >= 0)
    {
        current_takenNum -= n;
        std::cout << "[consume] tryConsume " << n << ", current: " << current_takenNum << std::endl;
        return true;
    }
    std::cout << "[consume] failed to tryConsume " << n << ", current: " << current_takenNum << std::endl;
    return false;
}

bool tokenManage::consumetoken(int n)
{
    std::unique_lock<std::mutex> lock(mtx);
    cond_.wait(lock, [this, n]() { return current_takenNum >= n; });
    current_takenNum -= n;
    std::cout << "[consume] consumetoken " << n << ", current: " << current_takenNum << std::endl;
    return true;
}

int tokenManage::getCurrentTakens()
{
    std::lock_guard<std::mutex> lock(mtx);
    return current_takenNum;
}
