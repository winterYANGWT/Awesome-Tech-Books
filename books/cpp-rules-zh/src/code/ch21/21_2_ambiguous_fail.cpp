// 第 21.2 节：两个互不包含的约束同时满足时，重载决议无法选出更严格的那个
// mode: fail
// expect: ambiguous
#include <string>

template <typename T>
concept HasName = requires(const T& value) { value.name(); };

template <typename T>
concept HasId = requires(const T& value) { value.id(); };

template <HasName T>
void describe(const T&) {}

template <HasId T>
void describe(const T&) {}

struct Employee {
    std::string name() const { return "Lin"; }
    int id() const { return 7; }
};

int main() {
    // Employee 同时满足 HasName 和 HasId，但两个概念互不包含。
    describe(Employee{});
}
