#include <iostream>
#include "tokenManage.h"
#include "consume.h"
#include "produce.h"
int main()
{
    tokenManage manage_node(200);
    produce produce_one(manage_node);
    produce produce_two(manage_node);
    produce produce_three(manage_node);

    consume consume_one(manage_node,5,5,nullptr);
    consume consume_two(manage_node,10,8,nullptr);
    consume consume_three(manage_node,2,20,nullptr);
    consume connsume_four(manage_node,20,2,nullptr);

    produce_one.start();
    produce_two.start();
    produce_three.start();

    consume_one.start();
    consume_two.start();
    consume_three.start();
    connsume_four.start();

     std::this_thread::sleep_for(std::chrono::hours(1)); 
}
