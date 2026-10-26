// 第 20.4 节：用 using 把模板名引进来再写不带限定的 friend，会声明出另一个函数
// mode: fail
#include <iostream>

namespace audit {
template<class T>
void purge(T* p) {
    delete p;
}
}  // namespace audit

using audit::purge;

class Session {
public:
    Session() = default;

private:
    ~Session() = default;

    // 不带限定、也不写 <>：这是声明一个全新的 ::purge(Session*)，不是模板特化
    friend void purge(Session*);
};

int main() {
    audit::purge(new Session);  // 模板没有拿到友元权限，析构函数仍是 private
}
