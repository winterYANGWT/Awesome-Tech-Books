// 第 29.6 节：用 Row 代理让 grid[r][c] 可用，并观察代理的边界
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

template <class T>
class Grid {
public:
    Grid(int rows, int cols)
        : rows_(rows), cols_(cols), data_(static_cast<std::size_t>(rows * cols)) {}

    // 代理：代表“一行”，本身不拥有元素，只记住这一行的起点和列数。
    // 用 const 参数区分只读代理和可写代理，避免从 const Grid 上偷出可写引用。
    template <bool IsConst>
    class RowProxy {
    public:
        using Pointer = std::conditional_t<IsConst, const T*, T*>;
        using Reference = std::conditional_t<IsConst, const T&, T&>;

        RowProxy(Pointer data, int cols) : data_(data), cols_(cols) {}

        Reference operator[](int col) const { return data_[col]; }
        int cols() const { return cols_; }

    private:
        Pointer data_;
        int cols_;
    };

    using Row = RowProxy<false>;
    using ConstRow = RowProxy<true>;

    Row operator[](int row) {
        return Row(data_.data() + static_cast<std::size_t>(row) * cols_, cols_);
    }

    ConstRow operator[](int row) const {
        return ConstRow(data_.data() + static_cast<std::size_t>(row) * cols_, cols_);
    }

    int rows() const { return rows_; }
    int cols() const { return cols_; }

private:
    int rows_;
    int cols_;
    std::vector<T> data_;
};

int main() {
    Grid<int> grid(3, 4);
    grid[1][2] = 42;                 // 第一次 [] 得到 Row，第二次 [] 得到 int&
    std::cout << "grid[1][2]=" << grid[1][2] << '\n';

    auto row = grid[2];              // 代理按值复制，仍然指向同一行
    row[0] = 7;
    std::cout << "row.cols()=" << row.cols() << '\n';
    std::cout << "grid[2][0]=" << grid[2][0] << '\n';

    const Grid<int>& cgrid = grid;
    std::cout << "const grid[1][2]=" << cgrid[1][2] << '\n';

    static_assert(std::is_same_v<decltype(grid[1][2]), int&>);
    static_assert(std::is_same_v<decltype(cgrid[1][2]), const int&>);
    static_assert(std::is_same_v<decltype(grid[1]), Grid<int>::Row>);

    // std::vector<bool> 是标准库自带的代理：operator[] 不返回 bool&。
    std::vector<bool> flags{true, false, true, false};
    auto bit = flags[1];             // 类型是 vector<bool>::reference，不是 bool&
    bit = true;
    const bool value = flags[1];
    std::cout << "vector<bool>: size=" << flags.size()
              << " flags[1]=" << std::boolalpha << value << '\n';
    static_assert(std::is_same_v<decltype(flags[1]), std::vector<bool>::reference>);
    static_assert(!std::is_same_v<decltype(flags[1]), bool&>);
}
