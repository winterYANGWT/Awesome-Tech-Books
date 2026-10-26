// 第 6.2 节：手写拷贝函数漏掉成员或基类时，编译器不会提醒
#include <iostream>
#include <string>
#include <utility>

// 客户记录：name 和 id 两个成员。手写的拷贝只记得 name。
class Customer {
public:
    Customer(std::string name, int id) : name_(std::move(name)), id_(id) {}

    Customer(const Customer& rhs) : name_(rhs.name_) {}   // 漏了 id_

    Customer& operator=(const Customer& rhs) {
        name_ = rhs.name_;                                 // 漏了 id_
        return *this;
    }

    const std::string& name() const { return name_; }
    int id() const { return id_; }

private:
    std::string name_;
    int id_ = 0;
};

// 基类故意提供默认构造，好让“派生类忘了调用基类拷贝”也能编过。
class Document {
public:
    Document() = default;
    explicit Document(std::string title) : title_(std::move(title)) {}
    Document(const Document&) = default;
    Document& operator=(const Document&) = default;

    const std::string& title() const { return title_; }
    void retitle(std::string title) { title_ = std::move(title); }

private:
    std::string title_;
};

class Contract : public Document {
public:
    Contract(std::string title, int pages)
        : Document(std::move(title)), pages_(pages) {}

    // 拷贝构造只写了 Document()，没有写 Document(rhs)：基类部分被默认构造。
    Contract(const Contract& rhs) : Document(), pages_(rhs.pages_) {}

    // 拷贝赋值没有调用 Document::operator=：基类部分保持原值。
    Contract& operator=(const Contract& rhs) {
        pages_ = rhs.pages_;
        return *this;
    }

    int pages() const { return pages_; }

private:
    int pages_ = 0;
};

// 正确写法：成员一个不少，基类显式调用对应的拷贝操作。
class FixedContract : public Document {
public:
    FixedContract(std::string title, int pages)
        : Document(std::move(title)), pages_(pages) {}

    FixedContract(const FixedContract&) = default;
    FixedContract& operator=(const FixedContract&) = default;

    int pages() const { return pages_; }

private:
    int pages_ = 0;
};

int main() {
    Customer original("Ada", 7);
    Customer copy(original);
    std::cout << "Customer 拷贝: name=" << copy.name()
              << " id=" << copy.id() << "（原对象 id=" << original.id() << "）\n";

    Customer assigned("Grace", 9);
    assigned = original;
    std::cout << "Customer 赋值: name=" << assigned.name()
              << " id=" << assigned.id() << "（原对象 id=" << original.id() << "）\n";

    Contract lease("Lease", 12);
    Contract broken_copy(lease);
    std::cout << "Contract 拷贝: title=\"" << broken_copy.title()
              << "\" pages=" << broken_copy.pages() << '\n';

    Contract broken_assign("Old", 1);
    broken_assign = lease;
    std::cout << "Contract 赋值: title=\"" << broken_assign.title()
              << "\" pages=" << broken_assign.pages() << '\n';

    FixedContract fixed("Lease", 12);
    FixedContract fixed_copy(fixed);
    FixedContract fixed_assign("Old", 1);
    fixed_assign = fixed;
    std::cout << "FixedContract: copy=\"" << fixed_copy.title()
              << "\" assign=\"" << fixed_assign.title() << "\"\n";
}
