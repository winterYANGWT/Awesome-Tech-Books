// 第 22.1 节：把与模板参数无关的代码抽离模板，避免每个尺寸一份实现
#include <cstddef>
#include <iostream>

// 版本 A：尺寸是模板参数，total() 的循环边界是编译期常量，
// 但 GridN<T, 4> 和 GridN<T, 64> 会各生成一份几乎相同的函数体。
template <class T, std::size_t N>
class GridN {
public:
    [[gnu::noinline]] T total() const {
        ++bodies_;  // 每个 GridN<T, N> 各有一个 bodies_
        T sum{};
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j)
                sum += cells_[i * N + j];
        return sum;
    }
    void fill(T value) {
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j)
                cells_[i * N + j] = value;
    }
    static long bodies() { return bodies_; }

private:
    T cells_[N * N]{};
    static inline long bodies_ = 0;
};

// 版本 B：只按元素类型参数化，尺寸变成构造函数的运行期参数。
// 同一个 GridBase<T> 的所有尺寸共享这一份 total_impl()。
template <class T>
class GridBase {
protected:
    GridBase(T* data, std::size_t n) : data_(data), n_(n) {}

    [[gnu::noinline]] T total_impl() const {
        ++bodies_;
        T sum{};
        for (std::size_t i = 0; i < n_; ++i)
            for (std::size_t j = 0; j < n_; ++j)
                sum += data_[i * n_ + j];
        return sum;
    }
    static long bodies() { return bodies_; }

private:
    T* data_;
    std::size_t n_;
    static inline long bodies_ = 0;
};

template <class T, std::size_t N>
class GridShared : private GridBase<T> {
public:
    GridShared() : GridBase<T>(cells_, N) {}

    T total() const { return this->total_impl(); }  // 内联薄包装，不复制算法
    void fill(T value) {
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = 0; j < N; ++j)
                cells_[i * N + j] = value;
    }
    using GridBase<T>::bodies;

private:
    T cells_[N * N]{};
};

int main() {
    GridN<int, 4> small;
    GridN<int, 64> large;
    small.fill(2);
    large.fill(2);

    std::cout << "GridN<int, 4>  sizeof = " << sizeof small
              << "  total = " << small.total() << '\n';
    std::cout << "GridN<int, 64> sizeof = " << sizeof large
              << "  total = " << large.total() << '\n';
    std::cout << "两个尺寸的 total() 实例各调用一次，各自的计数："
              << GridN<int, 4>::bodies() << " / " << GridN<int, 64>::bodies()
              << "（每个尺寸一份代码）\n";

    GridShared<int, 4> s_small;
    GridShared<int, 64> s_large;
    s_small.fill(2);
    s_large.fill(2);

    std::cout << "GridShared<int, 4>  sizeof = " << sizeof s_small
              << "  total = " << s_small.total() << '\n';
    std::cout << "GridShared<int, 64> sizeof = " << sizeof s_large
              << "  total = " << s_large.total() << '\n';
    std::cout << "共享实现被两个尺寸调用后的计数："
              << GridShared<int, 4>::bodies() << " / "
              << GridShared<int, 64>::bodies()
              << "（同一个 GridBase<int>::total_impl）\n";
}
