// 第 34.1 节：显式实例化只对列出的类型做“分离编译”
#pragma once

namespace shipping {

// 普通函数：调用方只需要声明，改实现只需重链接。
double base_rate(double kg);

// 模板：调用方也只需要声明；int 版由 quote.cpp 显式实例化并生成目标代码。
template <typename T>
T clamp_charge(T cents);

extern template int clamp_charge<int>(int);

}  // namespace shipping
