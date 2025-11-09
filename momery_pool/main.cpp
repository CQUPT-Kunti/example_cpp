#include "Fixed_size_pool.h"
#include <iostream>

int main()
{
    std::vector<Particle *> vec;
    vec.reserve(10000);
    for (int i = 0; i < 10000; i++)
    {
        Particle *p = new Particle{0, 0, 0, 0};
        vec.push_back(p);
    }

    for (auto *p : vec)
        delete p;

    std::cout << "BlockSize = " << q_pool.get_block_size()
              << ",BlockPerPage = " << q_pool.get_blocks_per_page() << std::endl;
    ;
}