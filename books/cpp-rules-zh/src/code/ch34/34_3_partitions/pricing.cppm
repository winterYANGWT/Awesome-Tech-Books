// 第 34.3 节：模块分区 :pricing，单独编译成一个 BMI
export module shop:pricing;

export namespace shop {

double subtotal(double unit_price, int count) {
    return unit_price * count;
}

}  // namespace shop
