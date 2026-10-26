// 第 6.2 节：把基类拷贝构造设为 protected，让按值切片在编译期失败
// mode: fail
// expect: protected
#include <string>

class Account {
public:
    Account() = default;
    virtual ~Account() = default;
    virtual const char* kind() const { return "Account"; }

protected:
    Account(const Account&) = default;
    Account& operator=(const Account&) = default;
};

class Savings : public Account {
public:
    const char* kind() const override { return "Savings"; }
};

void audit(Account account) {   // 按值接收基类
    (void)account;
}

int main() {
    Savings savings;
    audit(savings);   // 这里需要调用 Account 的拷贝构造，但它是 protected
}
