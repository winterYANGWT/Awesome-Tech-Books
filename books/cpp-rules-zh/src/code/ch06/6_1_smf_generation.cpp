// 第 6.1 节：编译器生成了哪些特殊成员函数，声明什么会让它们消失
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// 每个特殊成员函数被调用时记一笔，用来区分“真的移动”和“退化成拷贝”。
struct Counted {
    static inline int copies = 0;
    static inline int moves = 0;

    Counted() = default;
    Counted(const Counted&) { ++copies; }
    Counted(Counted&&) noexcept { ++moves; }
    Counted& operator=(const Counted&) { ++copies; return *this; }
    Counted& operator=(Counted&&) noexcept { ++moves; return *this; }
};

void reset() { Counted::copies = 0; Counted::moves = 0; }
void report(const char* name) {
    std::cout << name << ": copies=" << Counted::copies
              << " moves=" << Counted::moves << '\n';
}

// 什么都没有声明：六个特殊成员函数按需生成。
struct Plain {
    Counted value;
};

// 只声明析构函数：移动操作不再生成，拷贝操作仍然生成。
struct WithDtor {
    ~WithDtor() {}
    Counted value;
};

// 只声明拷贝构造：移动操作不再生成，拷贝赋值仍然独立生成。
struct WithCopy {
    WithCopy() = default;
    WithCopy(const WithCopy&) = default;
    Counted value;
};

// 声明移动构造：拷贝操作被定义为 deleted，移动赋值也不会生成。
struct WithMove {
    WithMove() = default;
    WithMove(WithMove&&) = default;
    Counted value;
};

// 成员函数模板从不抑制特殊成员函数；const 实参让非模板拷贝构造胜出。
struct WithTemplate {
    WithTemplate() = default;
    template <typename T>
    explicit WithTemplate(const T&) {}
    Counted value;
};

struct NoDefault {
    explicit NoDefault(int) {}
};

struct WithReference {
    int& ref;
};

struct WithConst {
    const int id;
};

static_assert(std::is_copy_constructible_v<Plain>);
static_assert(std::is_move_constructible_v<Plain>);
static_assert(!std::has_virtual_destructor_v<Plain>);   // 生成的析构不是虚的
static_assert(!std::is_default_constructible_v<NoDefault>);
static_assert(!std::is_copy_assignable_v<WithReference>);  // 引用不能重新绑定
static_assert(!std::is_copy_assignable_v<WithConst>);      // const 成员不能赋值
static_assert(std::is_copy_constructible_v<WithDtor>);   // 拷贝还在
static_assert(std::is_copy_assignable_v<WithDtor>);
static_assert(std::is_copy_assignable_v<WithCopy>);      // 两个拷贝操作互相独立
static_assert(std::is_move_constructible_v<WithMove>);
static_assert(!std::is_move_assignable_v<WithMove>);
static_assert(!std::is_copy_constructible_v<WithMove>);
static_assert(std::is_copy_constructible_v<WithTemplate>);
static_assert(std::is_move_constructible_v<WithTemplate>);  // 由拷贝构造兜底

int main() {
    {
        Plain a;
        Plain b(std::move(a));
        report("Plain（都交给编译器）");
    }
    reset();
    {
        WithDtor a;
        WithDtor b(std::move(a));
        report("WithDtor（有析构，move 退化成 copy）");
    }
    reset();
    {
        WithCopy a;
        WithCopy b(std::move(a));
        report("WithCopy（有拷贝构造，move 退化成 copy）");
    }
    reset();
    {
        WithMove a;
        WithMove b(std::move(a));
        report("WithMove（只声明移动构造）");
    }
    reset();
    {
        const WithTemplate a;
        WithTemplate b(a);   // 非模板拷贝构造优先于成员模板
        report("WithTemplate（成员模板不抑制拷贝构造）");
    }

    // 声明析构会让移动消失，但 = default 可以把想要的版本重新要回来。
    struct ExplicitFive {
        ExplicitFive() = default;
        ~ExplicitFive() = default;
        ExplicitFive(const ExplicitFive&) = default;
        ExplicitFive& operator=(const ExplicitFive&) = default;
        ExplicitFive(ExplicitFive&&) = default;
        ExplicitFive& operator=(ExplicitFive&&) = default;
        Counted value;
    };
    static_assert(std::is_nothrow_move_constructible_v<ExplicitFive>);
    reset();
    {
        ExplicitFive a;
        ExplicitFive b(std::move(a));
        report("ExplicitFive（= default 五件套）");
    }
}
