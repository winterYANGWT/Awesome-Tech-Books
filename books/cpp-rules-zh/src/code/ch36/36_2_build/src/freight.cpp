#include "freight.h"

namespace freight {

int shipping_cost(Carrier carrier, int weight_grams) {
    switch (carrier) {
    case Carrier::Bike:
        return weight_grams * 300 / 1000;
    case Carrier::Van:
        return 2000 + weight_grams * 100 / 1000;
    case Carrier::Drone:
        return weight_grams * 1200 / 1000;
    }
    return 0;  // 枚举值都在上面处理了；保留 return 让 -Wreturn-type 闭嘴
}

}  // namespace freight
