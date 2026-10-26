// 第 34.1 节：模板里的依赖名在定义处和实例化处两处查找
#include <iostream>

namespace warehouse {

struct Crate {
    int units;
};

template <typename T>
int units_of(const T& item) {
    // 依赖调用：先在定义处做普通查找，再在实例化处按实参类型做 ADL。
    // 这里的 describe 在模板定义之后才声明，普通查找看不到它。
    return describe(item);
}

int describe(const Crate& crate) {
    return crate.units;
}

}  // namespace warehouse

namespace depot {

struct Pallet {
    int units;
};

int describe(const Pallet& pallet) {
    return pallet.units * 10;
}

}  // namespace depot

int main() {
    std::cout << "Crate 单位数: " << warehouse::units_of(warehouse::Crate{7}) << '\n';
    std::cout << "Pallet 单位数: " << warehouse::units_of(depot::Pallet{7}) << '\n';
}
