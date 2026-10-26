// 第 33.4 节：using 声明取快照，using 指示在使用点解析
#include <iostream>

namespace rates {
double apply(double value) { return value * 10.0; }
}

namespace billing {
using rates::apply;                  // 快照：只看见 double 版本
int run() { return static_cast<int>(apply(1)); }
}

namespace rates {
int apply(int value) { return value + 100; }   // 在 using 声明之后才出现
}

namespace billing2 {
using rates::apply;                  // 现在两个重载都看见了
int run() { return apply(1); }
}

namespace units {
double convert(double value) { return value * 2.0; }
}

using namespace units;               // 指示：使用点之前可见的名字都进来

namespace units {
int convert(int value) { return value + 7; }
}

int main() {
    std::cout << "billing::run=" << billing::run() << '\n';
    std::cout << "billing2::run=" << billing2::run() << '\n';
    std::cout << "convert(1)=" << convert(1) << '\n';
}
