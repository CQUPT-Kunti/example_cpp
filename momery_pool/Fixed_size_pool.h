

#pragma once

#include <vector>

// 位运算 对齐边界
static inline std::size_t align_up(std::size_t n, std::size_t align)
{
    return (n + (align - 1)) & ~(align - 1);
}

class FixedSizePool
{
private:
    std::size_t block_size_;
    std::size_t blocks_per_page_;
    std::vector<void *> pages_;

    struct Node
    {
        Node *next;
    };
    Node *free_list_header; // 空闲小块头

    // 每次向系统申请一页内存，并把这一页内存切位很多小块，并把小块挂到空闲内存
    void expand()
    {
        // 一整页内存的字节数
        std::size_t page_bytes = block_size_ * blocks_per_page_;
        char *page = static_cast<char *>(::operator new[](page_bytes));
        pages_.push_back(page);

        // 拆分为小块

        for (std::size_t i = 0; i < blocks_per_page_; i++)
        {
            char *addr = page + i * block_size_;
            Node *n = reinterpret_cast<Node *>(addr);
            n->next = free_list_header;
            free_list_header = n;
        }
    }
    std::size_t adjust_block_size(std::size_t a)
    {
        std::size_t min = sizeof(void *);
        a = align_up(a < min ? min : a, alignof(void *));
        return a;
    }

public:
    explicit FixedSizePool(std::size_t block_size,
                           std::size_t blocks_per_page = 1024)
    {
        block_size_ = adjust_block_size(block_size);
        blocks_per_page_ = blocks_per_page;
        free_list_header = nullptr;
    }
    ~FixedSizePool()
    {
        for (void *p : pages_)
        {
            operator delete[](p);
        }
    }
    // 分配一小块内存
    void *allocate()
    {
        if (!free_list_header)
            expand();
        Node *head = free_list_header;
        free_list_header = head->next;
        return head;
    }
    void dealloccate(void *p)
    {
        if (!p)
        {
            return;
        }
        Node *node = static_cast<Node *>(p);
        node->next = free_list_header;
        free_list_header = node;
    } // 归还

    std::size_t get_block_size() const { return block_size_; }
    std::size_t get_blocks_per_page() const { return blocks_per_page_; }
    std::vector<void *> get_pages() const { return pages_; }
};

struct Particle
{
    float x, y, z;
    int life;

    void update()
    {
        ++life;
    }

    static void *operator new(std::size_t n);
    static void operator delete(void *p) noexcept;
};

static FixedSizePool q_pool(sizeof(Particle), 4096);

void *Particle::operator new(std::size_t n)
{
    return q_pool.allocate();
}

void Particle::operator delete(void *p) noexcept
{
    q_pool.dealloccate(p);
}