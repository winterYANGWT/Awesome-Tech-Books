// 第 20.1 节：泛型函数转调更窄的泛型函数，会把指针要求偷偷带进来
// mode: fail
#include <cstddef>

template<class T>
void destroy_at(T* p) {
    p->~T();
}

template<class FwdIt>
void destroy(FwdIt first, FwdIt last) {
    for (; first != last; ++first) {
        destroy_at(first);  // first 是迭代器，不是 T*
    }
}

// 一个不是指针的迭代器；不用标准容器，免得 ADL 把 std::destroy 也拉进来
struct IntRange {
    int data[3] = {1, 2, 3};

    struct Iterator {
        int* p;
        int& operator*() const { return *p; }
        Iterator& operator++() { ++p; return *this; }
        bool operator!=(const Iterator& other) const { return p != other.p; }
    };

    Iterator begin() { return {data}; }
    Iterator end() { return {data + 3}; }
};

int main() {
    IntRange values;
    ::destroy(values.begin(), values.end());
}
