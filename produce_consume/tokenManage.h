#pragma once

#include <mutex>
#include <condition_variable>

class tokenManage
{
private:
    int taken_maxNum = 0;
    int current_takenNum;
    std::mutex mtx;
    std::condition_variable cond_;

public:
    explicit tokenManage(int max_);
    bool addTaken();
    bool tryConsumeToken(int n);
    bool consumetoken(int n);
    int getCurrentTakens();
};
