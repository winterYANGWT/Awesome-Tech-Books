// 第 11.3 节：构造和析构期间，虚调用只到当前正在构造/析构的那一层
#include <iostream>
#include <string>
#include <typeinfo>
#include <utility>

class Transaction {
public:
    explicit Transaction(std::string log_info) : log_info_(std::move(log_info)) {
        std::cout << "  [Transaction ctor] typeid 是 Transaction："
                  << (typeid(*this) == typeid(Transaction)) << "\n";
        audit("构造");
        identify();                       // 虚调用：这里是 Transaction，不是 BuyTransaction
    }

    virtual ~Transaction() {
        std::cout << "  [~Transaction]\n";
        identify();                       // 派生部分已经销毁，仍然只到 Transaction
    }

    virtual std::string identify() const {
        std::cout << "    identify -> Transaction\n";
        return "Transaction";
    }

    void audit(const char* phase) const {  // 非虚：用派生类传上来的信息
        std::cout << "    audit(" << phase << "): " << log_info_ << "\n";
    }

private:
    std::string log_info_;
};

class BuyTransaction : public Transaction {
public:
    BuyTransaction() : Transaction(create_log_info()) {
        std::cout << "  [BuyTransaction ctor]\n";
    }

    ~BuyTransaction() override {
        std::cout << "  [~BuyTransaction]\n";
    }

    std::string identify() const override {
        std::cout << "    identify -> BuyTransaction\n";
        return "BuyTransaction";
    }

private:
    // static：不必碰还没构造的 BuyTransaction 成员
    static std::string create_log_info() { return "buy 100 shares"; }
};

int main() {
    std::cout << "--- 构造 BuyTransaction ---\n";
    BuyTransaction order;

    std::cout << "--- 构造完成之后 ---\n";
    Transaction& base = order;
    std::cout << "  typeid 是 BuyTransaction："
              << (typeid(base) == typeid(BuyTransaction)) << "\n";
    base.identify();                      // 现在才派发到 BuyTransaction

    std::cout << "--- 析构 ---\n";
}
