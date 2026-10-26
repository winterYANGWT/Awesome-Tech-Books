// 第 11.1 节：基类删掉默认构造、拷贝构造和拷贝赋值，把派生类的疏忽变成编译错误
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

class Identity {
public:
    explicit Identity(std::string id) : id_(std::move(id)) {}

    // 身份不能悄悄复制；需要换发时走具名操作，语义由派生类决定
    Identity(const Identity&) = delete;
    Identity& operator=(const Identity&) = delete;

    virtual ~Identity() = default;        // 这一项挡不住，必须让派生类可访问
    const std::string& id() const { return id_; }

protected:
    void reissue(std::string id) { id_ = std::move(id); }

private:
    std::string id_;
};

class Account : public Identity {
public:
    // 1. 默认构造：隐式版本被基类拖成 deleted；要构造对象就必须自己写，并传 id
    explicit Account(std::string id) : Identity(std::move(id)) {}

    // 2. 拷贝构造：基类删了，隐式版本也是 deleted
    // 3. 拷贝赋值：同上
    // 4. 析构：隐式版本合法，这一项管不住

    void rename(std::string id) { reissue(std::move(id)); }
};

static_assert(!std::is_default_constructible_v<Account>);
static_assert(!std::is_copy_constructible_v<Account>);
static_assert(!std::is_copy_assignable_v<Account>);
static_assert(!std::is_move_constructible_v<Account>);  // 拷贝被删，移动也跟着没了
static_assert(!std::is_move_assignable_v<Account>);
static_assert(std::is_destructible_v<Account>);   // 四个操作里只有析构挡不住

int main() {
    std::cout << "default=" << std::is_default_constructible_v<Account>
              << " copy=" << std::is_copy_constructible_v<Account>
              << " assign=" << std::is_copy_assignable_v<Account>
              << " move=" << std::is_move_constructible_v<Account>
              << " dtor=" << std::is_destructible_v<Account> << "\n";

    Account account("ACC-1001");
    account.rename("ACC-2002");
    std::cout << "id = " << account.id() << "\n";
}
