// 第 36.5 节：编译器零警告，clang-tidy 仍能指出三个模式问题
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Order {
    std::string id;
    int quantity;
};

int main() {
    std::vector<Order> orders{{"A-1", 2}, {"B-2", 0}};
    if (orders.size() == 0) {  // readability-container-size-empty
        return 0;
    }
    int* count = 0;  // modernize-use-nullptr
    (void)count;
    std::string id = orders.front().id;
    auto moved = std::move(id);
    std::cout << moved << ' ' << id << ' ' << orders.front().quantity << '\n';  // use-after-move
}
