// 第 34.5 节：模块在 global module fragment 里包含旧头文件
module;

#include "legacy_pricing.h"

export module pricing;

export namespace pricing {

double quote(int sku, int count) {
    return legacy::list_price(sku) * count;
}

}  // namespace pricing
