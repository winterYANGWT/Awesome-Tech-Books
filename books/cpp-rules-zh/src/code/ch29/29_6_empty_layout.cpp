// 第 29.6 节：空类至少 1 字节；EBO 和 [[no_unique_address]] 能省空间，但不是强制布局
#include <iostream>
#include <type_traits>

struct Empty {};

struct WithMember {
    Empty e;
    int value;
};

struct WithBase : Empty {
    int value;
};

struct NoUniqueMember {
    [[no_unique_address]] Empty e;
    int value;
};

struct TwoEmptyMembers {
    [[no_unique_address]] Empty a;
    [[no_unique_address]] Empty b;
    int value;
};

struct EmptyBase1 {};
struct EmptyBase2 {};

struct TwoEmptyBases : EmptyBase1, EmptyBase2 {
    int value;
};

struct Polymorphic {
    virtual ~Polymorphic() = default;
};

int main() {
    std::cout << std::boolalpha;
    std::cout << "sizeof(Empty)=" << sizeof(Empty)
              << " alignof(Empty)=" << alignof(Empty) << '\n';
    std::cout << "sizeof(WithMember)=" << sizeof(WithMember)
              << " (空成员占 1 字节加对齐)\n";
    std::cout << "sizeof(WithBase)=" << sizeof(WithBase)
              << " (空基类优化后通常只剩 int)\n";
    std::cout << "sizeof(NoUniqueMember)=" << sizeof(NoUniqueMember)
              << " ([[no_unique_address]] 空成员)\n";
    std::cout << "sizeof(TwoEmptyMembers)=" << sizeof(TwoEmptyMembers)
              << " (同类型空成员仍需不同地址，但可和 int 共用字节)\n";
    std::cout << "sizeof(TwoEmptyBases)=" << sizeof(TwoEmptyBases)
              << " (两个不同类型的空基类)\n";
    std::cout << "sizeof(Polymorphic)=" << sizeof(Polymorphic)
              << " is_empty=" << std::is_empty_v<Polymorphic> << '\n';

    static_assert(sizeof(Empty) >= 1);
    static_assert(std::is_empty_v<Empty>);
    static_assert(!std::is_empty_v<Polymorphic>);
}
