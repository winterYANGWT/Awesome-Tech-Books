// 第 34.3 节：没有 BMI 时，编译器不知道模块接口在哪里
// mode: fail
// expect: module 'shop' not found

import shop;

int main() {
    return shop::subtotal(12.5, 4) > 0 ? 0 : 1;
}
