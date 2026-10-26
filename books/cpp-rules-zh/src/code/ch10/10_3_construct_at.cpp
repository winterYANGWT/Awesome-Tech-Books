// 第 10.3 节：C++20 的 construct_at 绕过类级 placement new
#include <cstddef>
#include <iostream>
#include <memory>
#include <new>
#include <vector>

class Cell {
public:
    explicit Cell(int value) : value_(value) { std::cout << "  构造 Cell(" << value_ << ")\n"; }
    ~Cell() { std::cout << "  析构 Cell(" << value_ << ")\n"; }
    Cell(const Cell& other) : value_(other.value_) { std::cout << "  拷贝 Cell\n"; }
    int value() const { return value_; }

    // 普通 new 和类级 placement new 都被删除
    static void* operator new(std::size_t) = delete;
    static void* operator new(std::size_t, void*) = delete;

private:
    int value_;
};

int main() {
    std::cout << "1) vector 自己分配原始内存，用 construct_at 构造元素\n";
    std::vector<Cell> cells;
    cells.emplace_back(7);
    cells.emplace_back(8);
    std::cout << "  cells[1].value()=" << cells[1].value() << "\n";

    std::cout << "2) 直接调用 construct_at 也一样，不需要类级 placement new\n";
    alignas(Cell) std::byte buffer[sizeof(Cell)];
    Cell* p = std::construct_at(reinterpret_cast<Cell*>(buffer), 9);
    std::cout << "  p->value()=" << p->value() << "\n";
    std::destroy_at(p);
}
