// mode: crash
// expect: bad_alloc
// 没有长度上限：一条不换行的长行把缓冲撑到 1 MiB，分配器抛 bad_alloc。
#include <cstddef>
#include <cstdio>
#include <new>
#include <string>

struct LimitedAllocator {
    using value_type = char;
    static constexpr std::size_t kLimit = 2 * 1024 * 1024;

    LimitedAllocator() = default;
    template <class U> LimitedAllocator(const LimitedAllocator &) {}
    char *allocate(std::size_t n) {
        if (n > kLimit) throw std::bad_alloc();
        return static_cast<char *>(::operator new(n));
    }
    void deallocate(char *p, std::size_t) noexcept { ::operator delete(p); }
    template <class U> bool operator==(const LimitedAllocator &) const { return true; }
};

using LimitedString = std::basic_string<char, std::char_traits<char>, LimitedAllocator>;

int main() {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    LimitedString buffer;
    // ANCHOR: unbounded
    for (int i = 0; i < 300; ++i) {
        buffer.append(4096, 'A');  // 客户端一直发 'A'，不发换行
        if (buffer.size() % (256 * 1024) == 0)
            std::printf("buffer=%zu bytes\n", buffer.size());
    }
    // ANCHOR_END: unbounded
    std::printf("finished, buffer=%zu\n", buffer.size());
    return 0;
}
