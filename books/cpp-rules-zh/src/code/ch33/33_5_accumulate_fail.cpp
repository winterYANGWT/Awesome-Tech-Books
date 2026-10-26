// 第 33.5 节：运算符放错命名空间，std::accumulate 找不到
// mode: fail
// expect: operator+
#include <numeric>

namespace catalog {
struct Item {
    int cents;
};
}

int operator+(int total, catalog::Item item) {
    return total + item.cents;
}

int main() {
    catalog::Item items[3]{{100}, {200}, {300}};
    return std::accumulate(items, items + 3, 0);
}
