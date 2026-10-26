// 第 4.4 节：聚合初始化按声明顺序匹配，增删成员会悄悄改变含义
// flags: -Wno-missing-field-initializers
// 关掉“少写字段”的警告，是为了演示旧代码在结构体增员后含义怎么变；正文会说明这个警告。
#include <iostream>
#include <string>

// 旧接口：两个字段
struct LegacySample {
    int id;
    int value;
};

// 新接口：在中间插入 quality，原来的 {7, 3} 不再对应 value
struct NewSample {
    int id;
    int quality;
    int value;
};

struct Measurement {
    int day;
    int hour;
    double value;
};

int main() {
    std::cout << "--- 位置初始化按声明顺序 ---\n";
    LegacySample old_sample{7, 3};
    NewSample new_sample{7, 3};
    std::cout << "LegacySample: id=" << old_sample.id
              << ", value=" << old_sample.value << "\n";
    std::cout << "NewSample: id=" << new_sample.id
              << ", quality=" << new_sample.quality
              << ", value=" << new_sample.value << "\n";

    std::cout << "--- 没写出的成员被值初始化 ---\n";
    Measurement partial{1};
    int scores[5]{1, 2, 3};
    std::cout << "partial: day=" << partial.day << ", hour=" << partial.hour
              << ", value=" << partial.value << "\n";
    std::cout << "scores:";
    for (int score : scores) {
        std::cout << " " << score;
    }
    std::cout << "\n";

    std::cout << "--- C++20 指定初始化器把名字写出来 ---\n";
    Measurement named{.day = 1, .hour = 2, .value = 3.5};
    std::cout << "named: " << named.day << "/" << named.hour << "/" << named.value << "\n";
}
