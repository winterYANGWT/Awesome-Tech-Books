// 第 20.1 节：用 traits 给外部类型补信息，不要求它继承标记基类
#include <concepts>
#include <iostream>
#include <type_traits>

struct Cloneable {
    virtual ~Cloneable() = default;
};

struct Invoice : Cloneable {
    Invoice* Clone() const { return new Invoice(*this); }
    const char* kind() const { return "Invoice"; }
};

// 第三方类型：有 Clone，但不能改它的继承关系
struct Report {
    Report* Clone() const { return new Report(*this); }
    const char* kind() const { return "Report"; }
};

struct Snapshot {
    Snapshot* Clone() const { return new Snapshot(*this); }
    const char* kind() const { return "Snapshot"; }
};

// traits 的默认答案：只有继承 Cloneable 才算“支持克隆”
template<class T>
struct clone_traits {
    static constexpr bool supported = std::is_base_of_v<Cloneable, T>;
};

// 为外部类型补一条特化，原类型一行都不用改
template<>
struct clone_traits<Report> {
    static constexpr bool supported = true;
};

// concept 只看表达式：有没有 Clone()、返回值是不是 T*
template<class T>
concept HasClone = requires(const T& t) {
    { t.Clone() } -> std::same_as<T*>;
};

static_assert(std::is_base_of_v<Cloneable, Invoice>);
static_assert(!std::is_base_of_v<Cloneable, Report>);
static_assert(HasClone<Report>);
static_assert(HasClone<Snapshot>);
static_assert(clone_traits<Report>::supported);
static_assert(!clone_traits<Snapshot>::supported);

template<class T>
T* duplicate(const T& src) {
    if constexpr (clone_traits<T>::supported) {
        std::cout << "  clone_traits: 调用 Clone()\n";
        return src.Clone();
    } else {
        std::cout << "  clone_traits: 退回拷贝构造\n";
        return new T(src);
    }
}

int main() {
    Invoice invoice;
    Report report;
    Snapshot snapshot;

    std::cout << "Invoice:\n";
    Invoice* a = duplicate(invoice);
    std::cout << "  " << a->kind() << '\n';

    std::cout << "Report:\n";
    Report* b = duplicate(report);
    std::cout << "  " << b->kind() << '\n';

    std::cout << "Snapshot:\n";
    Snapshot* c = duplicate(snapshot);
    std::cout << "  " << c->kind() << '\n';

    delete a;
    delete b;
    delete c;
}
