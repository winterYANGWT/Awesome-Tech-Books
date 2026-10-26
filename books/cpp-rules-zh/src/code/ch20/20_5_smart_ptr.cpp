// 第 20.5 节：成员模板做广义拷贝，普通拷贝构造仍然要单独存在
#include <concepts>
#include <iostream>
#include <memory>
#include <type_traits>

struct Top {
    virtual ~Top() = default;
    virtual const char* name() const { return "Top"; }
};

struct Middle : Top {
    const char* name() const override { return "Middle"; }
};

struct Bottom : Middle {
    const char* name() const override { return "Bottom"; }
};

template<class T>
class SmartPtr {
public:
    explicit SmartPtr(T* p = nullptr) : p_(p) {}

    // 广义拷贝构造：只接受 U* 能转成 T* 的情况
    template<class U>
        requires std::convertible_to<U*, T*>
    SmartPtr(const SmartPtr<U>& other) : p_(other.p_) {
        ++generalized;
    }

    // 普通拷贝构造：同类型精确匹配时胜出，不会被成员模板取代
    SmartPtr(const SmartPtr&) = default;
    SmartPtr& operator=(const SmartPtr&) = default;

    template<class U>
        requires std::convertible_to<U*, T*>
    SmartPtr& operator=(const SmartPtr<U>& other) {
        p_ = other.p_;
        ++generalized_assigns;
        return *this;
    }

    T* get() const { return p_.get(); }
    T* operator->() const { return p_.get(); }

    static int generalized;
    static int generalized_assigns;

private:
    template<class>
    friend class SmartPtr;

    std::shared_ptr<T> p_;
};

template<class T>
int SmartPtr<T>::generalized = 0;

template<class T>
int SmartPtr<T>::generalized_assigns = 0;

static_assert(std::is_convertible_v<SmartPtr<Middle>, SmartPtr<Top>>);
static_assert(std::is_constructible_v<SmartPtr<Top>, SmartPtr<Bottom>>);
static_assert(!std::is_constructible_v<SmartPtr<Bottom>, SmartPtr<Top>>);
static_assert(std::is_copy_constructible_v<SmartPtr<int>>);

int main() {
    SmartPtr<Middle> middle(new Middle);

    SmartPtr<Top> top = middle;       // 广义拷贝构造
    SmartPtr<Top> copied = top;       // 普通拷贝构造
    SmartPtr<Top> assigned;
    assigned = middle;                // 广义赋值

    SmartPtr<const Top> const_top = top;  // Top* -> const Top*

    std::cout << "top = " << top->name() << '\n';
    std::cout << "copied = " << copied->name() << '\n';
    std::cout << "assigned = " << assigned->name() << '\n';
    std::cout << "const_top = " << const_top->name() << '\n';
    std::cout << "generalized = " << SmartPtr<Top>::generalized
              << ", generalized_assigns = " << SmartPtr<Top>::generalized_assigns
              << '\n';
}
