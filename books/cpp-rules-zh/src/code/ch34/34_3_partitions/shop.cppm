// 第 34.3 节：主模块接口把分区重新导出
export module shop;

export import :pricing;
export import :tax;

export namespace shop {

double packaging_fee(double net);

}  // namespace shop
