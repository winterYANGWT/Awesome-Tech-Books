// 第 10.4 节：容器的实际占用要按分配器取整后的块算
#include <cstddef>
#include <iostream>
#include <list>
#include <new>
#include <set>
#include <vector>

// 一个按 16 字节分档的池：每次分配向上取整，并记录实际拿走的字节
class RoundingPool {
public:
    void* allocate(std::size_t bytes) {
        std::size_t block = (bytes + 15) / 16 * 16;
        last_request_ = bytes;
        last_block_ = block;
        reserved_ += block;
        return ::operator new(block);
    }
    void deallocate(void* p) noexcept { ::operator delete(p); }

    std::size_t reserved() const { return reserved_; }
    std::size_t last_request() const { return last_request_; }
    std::size_t last_block() const { return last_block_; }

private:
    std::size_t reserved_ = 0;
    std::size_t last_request_ = 0;
    std::size_t last_block_ = 0;
};

template <class T>
struct PoolAllocator {
    using value_type = T;

    explicit PoolAllocator(RoundingPool& pool) : pool_(&pool) {}
    template <class U>
    PoolAllocator(const PoolAllocator<U>& other) : pool_(other.pool_) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(pool_->allocate(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) noexcept { pool_->deallocate(p); }

    template <class U>
    bool operator==(const PoolAllocator<U>& other) const noexcept { return pool_ == other.pool_; }
    template <class U>
    bool operator!=(const PoolAllocator<U>& other) const noexcept { return pool_ != other.pool_; }

    RoundingPool* pool_;
};

int main() {
    RoundingPool pool;

    std::cout << "1) vector<int>，1001 个元素\n";
    {
        std::size_t before = pool.reserved();
        std::vector<int, PoolAllocator<int>> v(PoolAllocator<int>{pool});
        v.resize(1001);
        std::size_t used = pool.reserved() - before;
        std::cout << "  请求 " << pool.last_request() << " 字节，取整后 "
                  << pool.last_block() << " 字节，共 " << used << " 字节（每元素约 4.01）\n";
    }

    std::cout << "2) list<char>，1000 个元素\n";
    {
        std::list<char, PoolAllocator<char>> values(PoolAllocator<char>{pool});
        std::size_t before = pool.reserved();
        for (int i = 0; i < 1000; ++i) values.push_back('x');
        std::size_t used = pool.reserved() - before;
        std::cout << "  节点请求 " << pool.last_request() << " 字节，取整后 "
                  << pool.last_block() << " 字节，共 " << used << " 字节（每元素 32）\n";
    }

    std::cout << "3) list<int>，1000 个元素\n";
    {
        std::list<int, PoolAllocator<int>> values(PoolAllocator<int>{pool});
        std::size_t before = pool.reserved();
        for (int i = 0; i < 1000; ++i) values.push_back(i);
        std::size_t used = pool.reserved() - before;
        std::cout << "  节点请求 " << pool.last_request() << " 字节，取整后 "
                  << pool.last_block() << " 字节，共 " << used << " 字节（每元素 32）\n";
    }

    std::cout << "4) set<int>，1000 个元素\n";
    {
        std::set<int, std::less<int>, PoolAllocator<int>> values(PoolAllocator<int>{pool});
        std::size_t before = pool.reserved();
        for (int i = 0; i < 1000; ++i) values.insert(i);
        std::size_t used = pool.reserved() - before;
        std::cout << "  节点请求 " << pool.last_request() << " 字节，取整后 "
                  << pool.last_block() << " 字节，共 " << used << " 字节（每元素 48）\n";
    }
}
