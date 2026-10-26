// 第 8.4 节：C API 的输出参数用 out_ptr / inout_ptr 接回智能指针
// std: c++23
#include <iostream>
#include <memory>

struct Session {
    int id;
};

// 模拟一个只会写裸指针的 C API：成功时把新对象写进 out。
extern "C" int session_open(Session** out, int id) {
    if (out == nullptr) {
        return -1;
    }
    *out = new Session{id};
    return 0;
}

// inout 风格：先看旧值，再替换。
extern "C" void session_reopen(Session** slot, int id) {
    delete *slot;
    *slot = new Session{id};
}

int main() {
    std::unique_ptr<Session> session;
    if (session_open(std::out_ptr(session), 5) != 0) {
        std::cout << "打开失败\n";
        return 1;
    }
    std::cout << "打开后 id = " << session->id << "\n";

    session_reopen(std::inout_ptr(session), 9);
    std::cout << "重开后 id = " << session->id << "\n";
    std::cout << "离开作用域时 unique_ptr 调用 delete\n";
}
