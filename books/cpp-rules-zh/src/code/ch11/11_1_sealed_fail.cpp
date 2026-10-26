// 第 11.1 节：派生类作者漏写拷贝构造时，编译在调用点失败
// mode: fail
// expect: deleted
#include <string>

class Identity {
public:
    explicit Identity(std::string id) : id_(std::move(id)) {}
    Identity(const Identity&) = delete;
    Identity& operator=(const Identity&) = delete;
    virtual ~Identity() = default;

private:
    std::string id_;
};

class Account : public Identity {
public:
    explicit Account(std::string id) : Identity(std::move(id)) {}
};

int main() {
    Account first("ACC-1");
    Account second = first;               // 隐式拷贝构造被基类的 deleted 版本拖下水
}
