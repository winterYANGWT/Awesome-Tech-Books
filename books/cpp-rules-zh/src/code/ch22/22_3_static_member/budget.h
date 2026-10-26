#pragma once

struct Budget {
    static const int limit = 100;    // 类内带初值：声明，不是定义
    static constexpr int cap = 200;  // C++17 起隐式 inline，本身就是定义
    static const double rate;        // 非整型，本来就要类外定义
};
