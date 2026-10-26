#include "freight.h"

#include <iostream>

int main() {
    int failures = 0;
    const auto check = [&failures](int actual, int expected, const char* what) {
        if (actual != expected) {
            std::cerr << "FAIL " << what << ": " << actual << " != " << expected << '\n';
            ++failures;
        }
    };

    check(freight::shipping_cost(freight::Carrier::Bike, 500), 150, "bike 500g");
    check(freight::shipping_cost(freight::Carrier::Bike, 1500), 450, "bike 1500g");
    check(freight::shipping_cost(freight::Carrier::Van, 500), 2050, "van 500g");
    check(freight::shipping_cost(freight::Carrier::Drone, 3000), 3600, "drone 3000g");

    if (failures == 0) {
        std::cout << "all 4 checks passed\n";
        return 0;
    }
    std::cout << failures << " checks failed\n";
    return 1;
}
