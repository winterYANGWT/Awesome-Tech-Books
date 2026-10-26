// 第 10.5 节：new T[n] 默认初始化，new T[n]() 才值初始化内置类型
#include <iostream>
#include <memory>

struct Tracer {
    inline static int constructed = 0;
    inline static int destroyed = 0;

    Tracer() { ++constructed; }
    Tracer(const Tracer&) { ++constructed; }
    ~Tracer() { ++destroyed; }
};

int main() {
    std::cout << "1) new int[5]()：值初始化，全部为 0\n";
    int* zeros = new int[5]();
    for (int i = 0; i < 5; ++i) std::cout << "  " << zeros[i];
    std::cout << "\n";
    delete[] zeros;

    std::cout << "2) new Tracer[4]：每个元素都默认构造\n";
    Tracer* tracers = new Tracer[4];
    std::cout << "  已构造：" << Tracer::constructed << "\n";
    delete[] tracers;
    std::cout << "  delete[] 后已析构：" << Tracer::destroyed << "\n";

    std::cout << "3) make_unique<int[]>(5)：同样值初始化\n";
    auto values = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) std::cout << "  " << values[i];
    std::cout << "\n";
}
