#pragma once
#include "tokenManage.h"
#include <thread>
class produce
{
private:
    tokenManage &tokenGroup;
    bool runState;
    std::thread go;

public:
    produce(tokenManage &manage);
    void start();
    void stop();
};