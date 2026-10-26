#pragma once

namespace freight {

enum class Carrier { Bike, Van, Drone };

// 返回运费（分）：bike 每公斤 3 元，van 起步 20 元，drone 每公斤 12 元
int shipping_cost(Carrier carrier, int weight_grams);

}  // namespace freight
