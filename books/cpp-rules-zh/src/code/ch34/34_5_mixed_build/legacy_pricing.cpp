#include "legacy_pricing.h"

namespace legacy {

double list_price(int sku) {
    return 10.0 + sku * 0.5;
}

}  // namespace legacy
