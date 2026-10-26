// 第 16.4 节：std::less 必须等价于 operator<，换次序另写具名比较器
#include <compare>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <type_traits>

struct Build {
    std::string name;
    int number;
    int footprint;

    // 按成员声明顺序生成 ==、<、<=、>、>=；比较结果由 <=> 决定
    auto operator<=>(const Build&) const = default;
};

// 需要按另一种次序时，给它一个名字，显式传给容器或算法
struct ByFootprint {
    bool operator()(const Build& lhs, const Build& rhs) const {
        return lhs.footprint < rhs.footprint;
    }
};

int main() {
    Build alpha{"alpha", 3, 120};
    Build beta{"beta", 1, 80};

    static_assert(std::is_same_v<decltype(alpha <=> beta), std::strong_ordering>);
    static_assert(std::is_same_v<decltype(alpha < beta), bool>);

    std::cout << std::boolalpha;
    std::cout << "alpha < beta: " << (alpha < beta) << '\n';
    std::cout << "std::less<Build>: " << std::less<Build>{}(alpha, beta) << '\n';
    std::cout << "alpha <= beta: " << (alpha <= beta) << '\n';
    std::cout << "alpha == alpha: " << (alpha == alpha) << '\n';

    std::set<Build> natural{alpha, beta};
    std::set<Build, ByFootprint> by_size{alpha, beta};

    std::cout << "默认次序:";
    for (const Build& build : natural) {
        std::cout << ' ' << build.name;
    }
    std::cout << '\n';

    std::cout << "按 footprint:";
    for (const Build& build : by_size) {
        std::cout << ' ' << build.name;
    }
    std::cout << '\n';
}
