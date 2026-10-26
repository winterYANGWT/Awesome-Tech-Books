// 第 36.1 节：一个函数里同时出现四种常见警告，先看懂再改代码
// mode: warn
// expect: -Wunused-parameter
#include <cstddef>
#include <iostream>
#include <vector>

enum class Carrier { Bike, Van, Drone };

const char* carrier_name(Carrier carrier) {
    switch (carrier) {  // 警告：Drone 没有处理
    case Carrier::Bike:
        return "bike";
    case Carrier::Van:
        return "van";
    }
}

// 计算运费：bike 按每公斤 3 元，van 起步 20 元，drone 按每公斤 12 元
int shipping_cost(Carrier carrier, int weight_grams, bool insured) {
    switch (carrier) {
    case Carrier::Bike:
        return weight_grams * 3 / 1000;
    case Carrier::Van:
        return 20 + weight_grams * 1 / 1000;
    case Carrier::Drone:
        return weight_grams * 12 / 1000;
    }
}  // 警告：控制流可能走到函数末尾，没有返回值

int main() {
    std::vector<int> weights{500, 1500, 3000};
    int total = 0;
    int parcel_count = 0;
    int warehouse_code = 7;  // 警告：定义了却没用
    for (int i = 0; i < weights.size(); ++i) {  // 警告：有符号与无符号比较
        total += shipping_cost(Carrier::Bike, weights[static_cast<std::size_t>(i)], false);
        ++parcel_count;
    }
    int average = total / parcel_count;
    std::cout << "total=" << total << " average=" << average << '\n';
}
