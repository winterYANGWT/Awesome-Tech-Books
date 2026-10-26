// 第 18.1 节：has-a 与 is-implemented-in-terms-of 都用成员表达
#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <type_traits>
#include <vector>

// 应用域：Customer 有一个 Address，而不是“是一个 Address”。
class Address {
public:
    Address(std::string street, std::string city)
        : street_(std::move(street)), city_(std::move(city)) {}

    const std::string& street() const { return street_; }
    const std::string& city() const { return city_; }

private:
    std::string street_;
    std::string city_;
};

class Customer {
public:
    Customer(std::string name, Address address)
        : name_(std::move(name)), address_(std::move(address)) {}

    void print() const {
        std::cout << name_ << " 住在 " << address_.street() << "，" << address_.city()
                  << '\n';
    }

private:
    std::string name_;
    Address address_;  // has-a
};

// 实现域：UniqueNames 用 list 实现，但它不是 list。
// 公开继承会把 list 的 push_back 一起暴露出去，重复元素就进来了。
class BadUniqueNames : public std::list<std::string> {
public:
    bool add(const std::string& name) {
        if (std::find(begin(), end(), name) != end()) {
            return false;
        }
        push_back(name);
        return true;
    }
};

class UniqueNames {
public:
    bool add(const std::string& name) {
        if (std::find(names_.begin(), names_.end(), name) != names_.end()) {
            return false;
        }
        names_.push_back(name);
        return true;
    }

    std::size_t size() const { return names_.size(); }

private:
    std::list<std::string> names_;  // is-implemented-in-terms-of
};

void showAsList(const std::list<std::string>& values) {
    for (const auto& value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';
}

int main() {
    Customer customer("Ada", Address("12 Signal St", "London"));
    customer.print();

    BadUniqueNames bad;
    bad.add("ada");
    bad.add("ada");
    std::cout << "BadUniqueNames::add 去重后 size=" << bad.size() << '\n';
    bad.push_back("ada");  // 继承来的接口绕过了 add 的规则
    std::cout << "再调用继承来的 push_back 后 size=" << bad.size() << "，内容：";
    showAsList(bad);

    UniqueNames good;
    std::cout << "UniqueNames add(ada)=" << good.add("ada")
              << " add(ada)=" << good.add("ada") << " size=" << good.size() << '\n';

    // 类型关系：BadUniqueNames 真是 list，UniqueNames 不是。
    static_assert(std::is_base_of_v<std::list<std::string>, BadUniqueNames>);
    static_assert(!std::is_base_of_v<std::list<std::string>, UniqueNames>);
    std::cout << "BadUniqueNames is-a list: true；UniqueNames is-a list: false\n";
}
