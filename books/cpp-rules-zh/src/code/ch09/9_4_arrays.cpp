// 第 9.4 节：数组所有权要用带 [] 的智能指针，析构次数由 delete[] 负责
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

struct Sample {
    Sample() : id(next++) {
        ++live;
        std::cout << "  构造 Sample " << id << "\n";
    }
    ~Sample() {
        --live;
        std::cout << "  析构 Sample " << id << "\n";
    }
    int id;
    static int live;
    static int next;
};
int Sample::live = 0;
int Sample::next = 0;

int main() {
    static_assert(std::is_same_v<decltype(std::declval<std::unique_ptr<Sample[]>&>()[0]),
                                 Sample&>);
    static_assert(!std::is_same_v<std::unique_ptr<Sample>,
                                  std::unique_ptr<Sample[]>>);

    std::cout << "== unique_ptr<T[]> ==\n";
    {
        auto samples = std::make_unique<Sample[]>(3);
        std::cout << "  元素个数由分配时决定，可以下标访问：samples[1].id = "
                  << samples[1].id << "\n";
        std::cout << "  离开作用域前存活 " << Sample::live << " 个\n";
    }
    std::cout << "  离开作用域后存活 " << Sample::live << " 个\n";

    std::cout << "== shared_ptr<T[]>（C++17）==\n";
    {
        std::shared_ptr<Sample[]> samples(new Sample[2]);
        std::cout << "  samples[0].id = " << samples[0].id
                  << "，引用计数 " << samples.use_count() << "\n";
    }
    std::cout << "  离开作用域后存活 " << Sample::live << " 个\n";

    std::cout << "== make_shared<T[]>（C++20）==\n";
    {
        auto samples = std::make_shared<Sample[]>(2);
        std::cout << "  samples[1].id = " << samples[1].id << "\n";
    }
    std::cout << "  离开作用域后存活 " << Sample::live << " 个\n";

    std::cout << "== vector 仍然是首选 ==\n";
    {
        std::vector<Sample> samples;
        samples.reserve(2);
        samples.emplace_back();
        samples.emplace_back();
        std::cout << "  vector 大小 " << samples.size()
                  << "，存活 " << Sample::live << " 个\n";
    }
    std::cout << "  最终存活 " << Sample::live << " 个\n";
}
