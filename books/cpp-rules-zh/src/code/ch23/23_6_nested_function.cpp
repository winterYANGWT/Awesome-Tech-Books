// 第 23.6 节：C++ 没有嵌套函数，用局部类或 lambda 把外层变量搬进来
#include <cstdio>
#include <vector>

// C++98 风格：局部类 + 引用成员，每加一个变量要同时改成员和构造调用
int totalWithLocalClass(int seed) {
    int total = seed;
    struct AddTwice {
        int& total;
        void operator()(int value) const { total += value * 2; }
    };
    AddTwice add{total};
    for (int value : {1, 2, 3}) {
        add(value);
    }
    return total;
}

// 今天：lambda 直接捕获外层变量，函数体就是“嵌套函数”
int totalWithLambda(int seed) {
    int total = seed;
    auto addTwice = [&total](int value) { total += value * 2; };
    for (int value : {1, 2, 3}) {
        addTwice(value);
    }
    return total;
}

int main() {
    std::printf("局部类的结果：%d\n", totalWithLocalClass(10));
    std::printf("lambda 的结果：%d\n", totalWithLambda(10));

    // C++98 不允许把局部类型用作模板实参，C++11 起允许
    struct Point {
        int x;
        int y;
    };
    std::vector<Point> points{{1, 2}, {3, 4}};
    std::printf("局部类型放进 vector：%d 个点\n", static_cast<int>(points.size()));
}
