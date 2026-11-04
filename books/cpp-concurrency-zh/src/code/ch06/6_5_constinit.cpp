// mode: run
// constinit 要求静态初始化；constexpr 函数可以在编译期把表算好。
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>

struct Config {
    int workers;
    int queue_capacity;
};

constexpr Config make_config() {
    return Config{8, 1024};
}

constexpr std::array<int, 8> make_squares() {
    std::array<int, 8> table{};
    for (int i = 0; i < 8; ++i) table[i] = i * i;
    return table;
}

constinit Config g_config = make_config();       // 静态初始化，main 之前就绪
constexpr std::array<int, 8> g_squares = make_squares();  // 编译期算好
constinit std::mutex g_config_mutex;             // 全局锁也不需要动态初始化
constinit std::atomic<int> g_requests{0};

static_assert(make_config().workers == 8);
static_assert(g_squares[7] == 49);

int main() {
    {
        std::lock_guard<std::mutex> lock(g_config_mutex);
        g_requests.fetch_add(1, std::memory_order_relaxed);
        std::cout << "配置：workers=" << g_config.workers
                  << " queue=" << g_config.queue_capacity << '\n';
        std::cout << "编译期表：squares[7]=" << g_squares[7] << '\n';
        std::cout << "全局 mutex 已就绪，请求数=" << g_requests.load() << '\n';
    }
}
