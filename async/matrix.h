#pragma once

#include <vector>
#include <memory>
#include <iostream>
#include <random>
#include <future>
#include <chrono>

typedef std::shared_ptr<std::vector<std::vector<double>>> ptr_v_v;

ptr_v_v initialization_matrix(int len)
{
    auto matrix = std::make_shared<std::vector<std::vector<double>>>(len, std::vector<double>(len));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0); // [0,1)之间的随机数

    for (int i = 0; i < len; ++i)
        for (int j = 0; j < len; ++j)
            (*matrix)[i][j] = dist(gen);

    return matrix;
}

void conv_block(const ptr_v_v &input, const ptr_v_v &kernel, ptr_v_v &output,
                int row_start, int row_end, int col_start, int col_end)
{
    int ksize = kernel->size();
    int len_out = output->size();
    int len_in = input->size();
    for (int i = row_start; i < row_end; ++i)
    {
        for (int j = col_start; j < col_end; ++j)
        {
            double sum = 0.0;
            for (int ki = 0; ki < ksize; ++ki)
            {
                for (int kj = 0; kj < ksize; ++kj)
                {
                    int ii = i + ki;
                    int jj = j + kj;
                    if (ii < len_in && jj < len_in)
                    {
                        sum += (*input)[ii][jj] * (*kernel)[ki][kj];
                    }
                }
            }
            (*output)[i][j] = sum;
        }
    }
}

void func(ptr_v_v one, int len1, ptr_v_v two, int len2)
{
    int output_len = len1 - len2 + 1;
    auto output = std::make_shared<std::vector<std::vector<double>>>(output_len, std::vector<double>(output_len, 0.0));
    auto output1 = std::make_shared<std::vector<std::vector<double>>>(output_len, std::vector<double>(output_len, 0.0));

    int mid_row = output_len / 2;
    int mid_col = output_len / 2;

    // 异步卷积
    auto start = std::chrono::high_resolution_clock::now();
    auto f1 = std::async(std::launch::async, conv_block, one, two, std::ref(output), 0, mid_row, 0, mid_col);
    auto f2 = std::async(std::launch::async, conv_block, one, two, std::ref(output), 0, mid_row, mid_col, output_len);
    auto f3 = std::async(std::launch::async, conv_block, one, two, std::ref(output), mid_row, output_len, 0, mid_col);
    auto f4 = std::async(std::launch::async, conv_block, one, two, std::ref(output), mid_row, output_len, mid_col, output_len);
    f1.get();
    f2.get();
    f3.get();
    f4.get();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "异步卷积耗时: " << duration_us << " μs" << std::endl;

    // 单线程卷积
    auto start1 = std::chrono::high_resolution_clock::now();
    conv_block(one, two, output1, 0, output_len, 0, output_len);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration_single = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1).count();
    std::cout << "单卷积耗时: " << duration_single << " μs" << std::endl;
}
