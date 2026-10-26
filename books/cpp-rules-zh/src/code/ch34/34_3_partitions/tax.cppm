// 第 34.3 节：模块分区 :tax，和 :pricing 互不依赖
export module shop:tax;

export namespace shop {

double with_tax(double amount) {
    return amount * 1.13;
}

}  // namespace shop
