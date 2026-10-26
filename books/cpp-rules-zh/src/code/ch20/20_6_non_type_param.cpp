// 第 20.6 节：非类型模板参数把长度和起点编进类型
#include <iostream>
#include <type_traits>

template<class T, int N, int Start = 0>
class FixedArray {
public:
    static constexpr int size = N;
    static constexpr int start = Start;

    FixedArray() { ++live; }
    ~FixedArray() { --live; }

    T& operator[](int i) { return data_[i]; }
    const T& operator[](int i) const { return data_[i]; }

    static int live;

private:
    T data_[N]{};
};

template<class T, int N, int S>
int FixedArray<T, N, S>::live = 0;

template<int N>
int sum_first(const FixedArray<int, N>& values) {
    int total = 0;
    for (int i = 0; i < N; ++i) {
        total += values[i];
    }
    return total;
}

static_assert(FixedArray<int, 4>::size == 4);
static_assert(FixedArray<int, 4>::start == 0);
static_assert(FixedArray<int, 4, 10>::start == 10);
static_assert(!std::is_same_v<FixedArray<int, 4>, FixedArray<int, 8>>);
static_assert(!std::is_assignable_v<FixedArray<int, 4>&, const FixedArray<int, 8>&>);

int main() {
    FixedArray<int, 4> four;
    FixedArray<int, 8> eight;
    FixedArray<int, 4> another_four;

    for (int i = 0; i < 4; ++i) {
        four[i] = i + 1;
    }
    for (int i = 0; i < 8; ++i) {
        eight[i] = i + 1;
    }

    std::cout << "sizeof four = " << sizeof(four)
              << ", sizeof eight = " << sizeof(eight) << '\n';
    std::cout << "live<4> = " << FixedArray<int, 4>::live
              << ", live<8> = " << FixedArray<int, 8>::live << '\n';
    std::cout << "sum_first(four) = " << sum_first(four) << '\n';
    std::cout << "start<4,10> = " << FixedArray<int, 4, 10>::start << '\n';
}
