// 第 31.4 节：在 std::execution 可用之前，用 std::async/future 把异步步骤串起来
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

struct Config {
    int tax_percent;
};

struct Quote {
    int unit_price;
    int quantity;
};

int main() {
    std::thread::id main_id = std::this_thread::get_id();

    auto config_future = std::async(std::launch::async, [main_id] {
        std::cout << "load_config on main thread? "
                  << (std::this_thread::get_id() == main_id ? "true" : "false") << "\n";
        return Config{8};
    });

    Config cfg = config_future.get();  // 必须在这里等，后面的任务才能拿到配置

    auto quote_future = std::async(std::launch::async, [main_id, cfg] {
        std::cout << "load_quote on main thread? "
                  << (std::this_thread::get_id() == main_id ? "true" : "false") << "\n";
        return Quote{199, 3 + cfg.tax_percent};
    });

    Quote quote = quote_future.get();  // 又一次阻塞等待
    int total = quote.unit_price * quote.quantity;
    std::cout << "total=" << total << "\n";

    auto failing = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("inventory service down");
    });
    try {
        int value = failing.get();
        std::cout << "unexpected value " << value << "\n";
    } catch (const std::exception& e) {
        std::cout << "caught from future: " << e.what() << "\n";
    }
}
