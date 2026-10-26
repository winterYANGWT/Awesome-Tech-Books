// 第 16.7 节：operator-> 沿代理链递归，直到拿到裸指针
#include <iostream>
#include <string>

struct Payload {
    std::string name;
    int level;

    void show() const { std::cout << name << ':' << level << '\n'; }
};

// 第一层代理：只负责把指针交出去
class Ref {
public:
    explicit Ref(Payload* payload) : payload_(payload) {}

    Payload* operator->() const { return payload_; }
    Payload& operator*() const { return *payload_; }

private:
    Payload* payload_;
};

// 第二层代理：operator-> 返回 Ref，编译器再对 Ref 调用一次 operator->
class Handle {
public:
    explicit Handle(Payload* payload) : payload_(payload) {}

    Ref operator->() const { return Ref(payload_); }
    Payload& operator*() const { return *payload_; }

private:
    Payload* payload_;
};

int main() {
    Payload payload{"sensor", 3};
    Handle handle(&payload);

    handle->show();               // (handle.operator->())->show()，两跳后到达 Payload*
    (*handle).show();

    Ref ref = handle.operator->();
    ref->show();                  // 只经过 Ref 的一跳

    std::cout << "name: " << handle->name << '\n';
}
