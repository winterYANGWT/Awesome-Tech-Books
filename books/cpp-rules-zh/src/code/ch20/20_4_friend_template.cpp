// 第 20.4 节：给另一个命名空间里的函数模板特化授友元，要写出模板实参
#include <iostream>

namespace audit {
template<class T>
void purge(T* p) {
    delete p;
    std::cout << "purged\n";
}
}  // namespace audit

class Session {
public:
    Session() = default;

private:
    ~Session() = default;

    // 显式写出 <>：friend 是 audit::purge<Session> 这个特化
    friend void audit::purge<>(Session*);
};

int main() {
    audit::purge(new Session);
}
