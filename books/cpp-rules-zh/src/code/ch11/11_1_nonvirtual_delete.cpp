// 第 11.1 节：非虚析构的基类指针删除派生对象，派生类部分不会析构
// mode: crash
// expect: new-delete-type-mismatch
#include <string>

struct Session {
    ~Session() = default;                 // 非虚：delete Session* 是未定义行为
};

struct AuditSession : Session {
    // 派生类自己的资源：长度超过 SSO 缓冲区，必然在堆上分配
    std::string trail = std::string(128, 'x');
};

int main() {
    Session* session = new AuditSession;
    delete session;                       // 只调用 ~Session，trail 的缓冲区泄漏
}
