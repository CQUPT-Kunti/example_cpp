#include "matrix.h"

// 异步卷积耗时: 3007 μs
// 单卷积耗时: 12891 μs

int main()
{
    int len = 1024;
    int ksize = 3;

    auto A = initialization_matrix(len);
    auto K = initialization_matrix(ksize);

    func(A, len, K, ksize);

    return 0;
}