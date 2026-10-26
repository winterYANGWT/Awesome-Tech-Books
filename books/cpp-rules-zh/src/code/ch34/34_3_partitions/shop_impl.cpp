// 第 34.3 节：模块实现单元，函数体不进入 BMI
module shop;

namespace shop {

double packaging_fee(double net) {
    return net < 100.0 ? 5.0 : 0.0;
}

}  // namespace shop
