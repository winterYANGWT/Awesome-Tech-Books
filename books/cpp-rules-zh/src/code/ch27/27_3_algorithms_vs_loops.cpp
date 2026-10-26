// 第 27.3 节：把一次任务拆成有名字的算法，而不是一个什么都做的循环
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

struct Order {
    int id;
    int cents;
    bool paid;
};

bool is_big_paid(const Order& o) {
    return o.paid && o.cents >= 10000;
}

int main() {
    std::vector<Order> orders{
        {101, 12000, true}, {102, 800, true}, {103, 25000, false},
        {104, 15000, true}, {105, 300, false}, {106, 9900, true},
        {107, 18000, true}, {108, 5000, false},
    };

    // 手写循环：读的人要同时跟踪“总额”“大额订单 id”“循环边界”三件事。
    int loop_total = 0;
    std::vector<int> loop_ids;
    for (std::size_t i = 0; i != orders.size(); ++i) {
        if (orders[i].paid && orders[i].cents >= 10000) {
            loop_total += orders[i].cents;
            loop_ids.push_back(orders[i].id);
        }
    }

    // 算法组合：每个调用点只回答一个问题。
    int algo_total = std::accumulate(
        orders.begin(), orders.end(), 0,
        [](int sum, const Order& o) { return is_big_paid(o) ? sum + o.cents : sum; });

    std::vector<Order> big_orders;
    std::ranges::copy_if(orders, std::back_inserter(big_orders), is_big_paid);
    std::vector<int> algo_ids;
    std::ranges::transform(big_orders, std::back_inserter(algo_ids), &Order::id);

    auto first_big = std::find_if(orders.begin(), orders.end(), is_big_paid);
    std::size_t paid_count = std::count_if(
        orders.begin(), orders.end(), [](const Order& o) { return o.paid; });
    bool has_unpaid_big = std::any_of(
        orders.begin(), orders.end(), [](const Order& o) { return !o.paid && o.cents >= 20000; });

    std::cout << "手写循环：总额 " << loop_total << "，id";
    for (int id : loop_ids) std::cout << ' ' << id;
    std::cout << '\n';

    std::cout << "算法组合：总额 " << algo_total << "，id";
    for (int id : algo_ids) std::cout << ' ' << id;
    std::cout << '\n';

    std::cout << "第一个大额已付订单 id=" << first_big->id
              << "，已付订单数=" << paid_count
              << "，存在大额未付？" << (has_unpaid_big ? "是" : "否") << '\n';
}
