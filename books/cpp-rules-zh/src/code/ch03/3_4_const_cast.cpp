// 第 3.4 节：const_cast 的合法用法——底层对象本来不是 const
#include <cstdio>
#include <string>
#include <utility>

// 旧接口不接受 const，但它只读不写
void legacy_log(char* text) {
    std::printf("log: %s\n", text);
}

void show(const std::string& text) {
    std::printf("const: %s\n", text.c_str());
}

void show(std::string& text) {
    std::printf("mutable: %s\n", text.c_str());
}

class Account {
public:
    explicit Account(std::string id) : id_(std::move(id)) {}

    const std::string& id() const { return id_; }

    // 非 const 重载转调 const 重载，再只对返回值去 const；对象本身不是 const
    std::string& id() {
        return const_cast<std::string&>(std::as_const(*this).id());
    }

private:
    std::string id_;
};

int main() {
    char buffer[] = "startup";
    const char* view = buffer;

    // buffer 是可写的数组，view 只是它的只读视图，去 const 后写回是合法的
    legacy_log(const_cast<char*>(view));

    std::string name = "worker";
    show(std::as_const(name));   // 选 const 重载，不需要写 const_cast
    show(name);                  // 选非 const 重载

    Account account{"A-17"};
    account.id() += "-x";
    std::printf("account: %s\n", account.id().c_str());
}
