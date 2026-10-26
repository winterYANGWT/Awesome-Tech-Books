#include "quote.h"

namespace shipping {

double base_rate(double kg) {
    return kg < 1.0 ? 5.0 : 5.0 + (kg - 1.0) * 2.0;
}

template <typename T>
T clamp_charge(T cents) {
    const T minimum = T{100};
    return cents < minimum ? minimum : cents;
}

// 显式实例化定义：只生成 int 这一份目标代码。
template int clamp_charge<int>(int);

}  // namespace shipping
