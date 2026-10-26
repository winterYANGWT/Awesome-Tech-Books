// 第 16.3 节：重载 && 之后没有短路，左侧为空时右侧仍然解引用
// mode: crash
// expect: member access within null pointer
#include <iostream>

struct Node {
    int value;
};

class Ptr {
public:
    explicit Ptr(Node* node) : node_(node) {}
    explicit operator bool() const { return node_ != nullptr; }
    Node* operator->() const { return node_; }

private:
    Node* node_;
};

// 看起来像内置的 &&，实际是普通函数：两个实参先求值，再进入函数体
bool operator&&(const Ptr& ptr, bool right) {
    return static_cast<bool>(ptr) && right;
}

int main() {
    Ptr ptr(nullptr);

    // 内置 && 会在 ptr 为空时跳过右边；重载版本不会
    if (ptr && ptr->value > 0) {
        std::cout << "可到达\n";
    }
}
