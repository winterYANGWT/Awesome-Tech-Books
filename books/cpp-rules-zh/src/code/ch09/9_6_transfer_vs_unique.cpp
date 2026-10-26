// 第 9.6 节：拷贝即转移的老式智能指针会掏空源对象；unique_ptr 用移动表达同一件事
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// 一个模仿 auto_ptr 语义的最小类型：拷贝构造和拷贝赋值都接管所有权。
template <typename T>
class TransferPtr {
public:
    explicit TransferPtr(T* p = nullptr) : p_(p) {}
    ~TransferPtr() { delete p_; }

    // 注意：参数是非 const 左值引用，因为拷贝会修改源对象。
    TransferPtr(TransferPtr& other) : p_(other.p_) { other.p_ = nullptr; }

    TransferPtr& operator=(TransferPtr& other) {
        if (this != &other) {
            delete p_;
            p_ = other.p_;
            other.p_ = nullptr;
        }
        return *this;
    }

    T& operator*() const { return *p_; }
    T* operator->() const { return p_; }
    T* get() const { return p_; }

private:
    T* p_;
};

struct Item {
    explicit Item(std::string n) : name(std::move(n)) {}
    std::string name;
};

int main() {
    std::cout << "== 拷贝即转移 ==\n";
    {
        TransferPtr<Item> first(new Item("订单"));
        TransferPtr<Item> second(first);   // first 被掏空
        std::cout << "first 是否为空：" << std::boolalpha
                  << (first.get() == nullptr) << "\n";
        std::cout << "second 指向：" << second->name << "\n";
        // 这里不能再通过 first 访问对象；它已经不再拥有任何东西。
    }

    std::cout << "== unique_ptr 放进容器并排序 ==\n";
    {
        std::vector<std::unique_ptr<Item>> items;
        items.push_back(std::make_unique<Item>("橙子"));
        items.push_back(std::make_unique<Item>("苹果"));
        items.push_back(std::make_unique<Item>("香蕉"));

        std::sort(items.begin(), items.end(),
                  [](const std::unique_ptr<Item>& lhs,
                     const std::unique_ptr<Item>& rhs) {
                      return lhs->name < rhs->name;
                  });

        for (const auto& item : items) {
            std::cout << "  " << item->name << "\n";
        }
    }
}
