// mode: run
// wait_for 接受 duration，wait_until 接受 time_point；high_resolution_clock 不一定单调。
#include <chrono>
#include <future>
#include <iostream>

using namespace std::chrono_literals;

const char* status_name(std::future_status s) {
    switch (s) {
    case std::future_status::ready: return "ready";
    case std::future_status::timeout: return "timeout";
    case std::future_status::deferred: return "deferred";
    }
    return "unknown";
}

int main() {
    std::promise<int> p;
    auto f = p.get_future(); // 一直不 set_value，等一下必然超时

    std::cout << "wait_for(100ms): " << status_name(f.wait_for(100ms)) << '\n';
    std::cout << "wait_until(now()+100ms): "
              << status_name(f.wait_until(std::chrono::steady_clock::now() + 100ms))
              << '\n';
    std::cout << "high_resolution_clock::is_steady="
              << std::chrono::high_resolution_clock::is_steady << '\n';
}
