// 第 12.3 节：用成员实现对象和 copy-and-swap 做不抛的提交
// mode: run
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Contact {
    std::string name;
    std::string email;
};

// 真正干活的对象：两张表必须一起更新，任何一步失败都可能让它们对不上
class DirectoryImpl {
public:
    void add(const Contact& contact) {
        by_name_.insert_or_assign(contact.name, contact);
        if (contact.name == "poison") {
            throw std::runtime_error("address book is full");
        }
        order_.push_back(contact.name);
    }

    std::size_t count() const noexcept { return by_name_.size(); }

    bool consistent() const noexcept { return by_name_.size() == order_.size(); }

    std::string names() const {
        std::string result;
        for (const std::string& name : order_) {
            if (!result.empty()) {
                result += ", ";
            }
            result += name;
        }
        return result;
    }

    void swap(DirectoryImpl& other) noexcept {
        using std::swap;
        swap(by_name_, other.by_name_);
        swap(order_, other.order_);
    }

private:
    std::map<std::string, Contact> by_name_;
    std::vector<std::string> order_;
};

// 对外接口：只负责在副本上完成工作，然后用不抛的 swap 提交
class Directory {
public:
    Directory() = default;
    Directory(const Directory&) = default;
    Directory(Directory&&) noexcept = default;

    // 按值传参：复制/移动发生在进入函数之前，函数体只做不抛的提交
    Directory& operator=(Directory other) noexcept {
        swap(other);
        return *this;
    }

    void swap(Directory& other) noexcept { impl_.swap(other.impl_); }

    // 反例：直接把可能失败的两步暴露给调用方
    void add_unsafe(const Contact& contact) { impl_.add(contact); }

    // 正例：先在 temp 上做完所有可能抛的工作，成功后再交换
    void add(const Contact& contact) {
        Directory temp(*this);
        temp.impl_.add(contact);
        swap(temp);
    }

    std::size_t count() const noexcept { return impl_.count(); }
    bool consistent() const noexcept { return impl_.consistent(); }
    std::string order() const { return impl_.names(); }

private:
    DirectoryImpl impl_;
};

void swap(Directory& a, Directory& b) noexcept { a.swap(b); }

static_assert(std::is_nothrow_swappable_v<Directory>);
static_assert(std::is_nothrow_move_assignable_v<Directory>);

void assign_from(Directory& target, const Directory& source) {
    target = source;
}

int main() {
    Directory unsafe;
    unsafe.add_unsafe({"ada", "ada@example.com"});
    unsafe.add_unsafe({"bob", "bob@example.com"});
    try {
        unsafe.add_unsafe({"poison", "poison@example.com"});
    } catch (const std::exception& e) {
        std::printf("unsafe failed: %s\n", e.what());
    }
    std::printf("unsafe: count=%zu consistent=%d order=[%s]\n", unsafe.count(),
                unsafe.consistent() ? 1 : 0, unsafe.order().c_str());

    Directory safe;
    safe.add({"ada", "ada@example.com"});
    safe.add({"bob", "bob@example.com"});
    try {
        safe.add({"poison", "poison@example.com"});
    } catch (const std::exception& e) {
        std::printf("strong failed: %s\n", e.what());
    }
    std::printf("strong: count=%zu consistent=%d order=[%s]\n", safe.count(),
                safe.consistent() ? 1 : 0, safe.order().c_str());

    assign_from(safe, safe);  // 通过别名发生的自赋值，copy-and-swap 天然处理
    std::printf("after self-assignment: count=%zu consistent=%d\n", safe.count(),
                safe.consistent() ? 1 : 0);

    safe.add({"carol", "carol@example.com"});
    std::printf("after retry: count=%zu consistent=%d order=[%s]\n", safe.count(),
                safe.consistent() ? 1 : 0, safe.order().c_str());
}
