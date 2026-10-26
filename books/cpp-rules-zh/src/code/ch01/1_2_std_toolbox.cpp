// 第 1.2 节：TR1 里的几件工具如今都在 std 里：function、unordered_map、
// shared_ptr/weak_ptr、tuple、array

#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

struct Node {
    explicit Node(std::string value) : name(std::move(value)) {}

    std::string name;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> parent;
};

int main() {
    // tr1::function 进了 C++11，现在叫 std::function。
    std::unordered_map<std::string, std::function<int(int)>> operations;
    operations["double"] = [](int value) { return value * 2; };
    operations["square"] = [](int value) { return value * value; };

    std::cout << "function: " << operations["double"](21) << ' '
              << operations["square"](7) << '\n';

    // tr1::shared_ptr / tr1::weak_ptr 也进了 C++11。
    // parent 用 weak_ptr，避免 a 和 b 互相持有形成环。
    auto a = std::make_shared<Node>("a");
    auto b = std::make_shared<Node>("b");
    a->next = b;
    b->parent = a;

    std::cout << "b use_count before: " << b.use_count() << '\n';
    a.reset();
    std::cout << "b use_count after: " << b.use_count() << '\n';
    std::cout << "parent expired: " << b->parent.expired() << '\n';

    // tr1::tuple 和 tr1::array 同样在 C++11 转正。
    const std::tuple<int, std::string, double> item{7, "widget", 12.5};
    const auto [id, label, price] = item;

    const std::array<int, 3> squares{1, 4, 9};

    std::cout << "tuple: " << id << ' ' << label << ' ' << price << '\n';
    std::cout << "array: " << squares[0] << ' ' << squares[2] << '\n';
}
