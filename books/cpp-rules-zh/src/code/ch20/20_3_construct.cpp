// 第 20.3 节：把“构造什么”和“用什么初值”分开，并让形参按类类型对待
#include <cstddef>
#include <iostream>
#include <new>
#include <type_traits>

template<class T1, class T2>
T1* construct(T1* p, const T2& value) {
    // placement new：不申请内存，在 p 上以 direct-initialization 构造
    return ::new (static_cast<void*>(p)) T1(value);
}

template<class T>
void destroy_at(T* p) {
    p->~T();
}

template<class T>
struct Storage {
    alignas(T) std::byte bytes[sizeof(T)];

    T* get() { return reinterpret_cast<T*>(bytes); }
};

struct Base {
    int id;
};

struct Derived : Base {
    int extra;
};

struct Token {
    explicit Token(int v) : value(v) {}
    int value;
};

// ESS-11：不知道 T 是不是类，形参默认用 const&，成员在初始化列表里构造
struct Matrix {
    static int copies;
    static int defaults;
    Matrix() { ++defaults; }
    Matrix(const Matrix&) { ++copies; }
    Matrix& operator=(const Matrix&) { ++copies; return *this; }
};

int Matrix::copies = 0;
int Matrix::defaults = 0;

template<class T>
struct Node {
    T value;
    explicit Node(const T& v) : value(v) {}  // 一次拷贝构造
};

template<class T>
struct NodeAssigned {
    T value;
    explicit NodeAssigned(const T& v) { value = v; }  // 默认构造 + 赋值
};

int main() {
    double d = 0;
    construct(&d, 42);  // T1 = double，T2 = int，窄化发生在构造函数里
    std::cout << "double = " << d << '\n';

    Storage<Base> base_storage;
    Base* pb = construct(base_storage.get(), Derived{{7}, 99});
    std::cout << "Base.id = " << pb->id
              << ", sizeof(Base) = " << sizeof(Base)
              << ", sizeof(Derived) = " << sizeof(Derived) << '\n';
    destroy_at(pb);

    Storage<Token> token_storage;
    Token* pt = construct(token_storage.get(), 42);  // explicit 构造也能用
    std::cout << "Token.value = " << pt->value << '\n';
    destroy_at(pt);

    Matrix source;
    Matrix::copies = 0;
    Matrix::defaults = 0;
    Node<Matrix> with_init(source);
    std::cout << "init-list: default=" << Matrix::defaults
              << " copy=" << Matrix::copies << '\n';

    Matrix::copies = 0;
    Matrix::defaults = 0;
    NodeAssigned<Matrix> with_assign(source);
    std::cout << "body-assign: default=" << Matrix::defaults
              << " copy=" << Matrix::copies << '\n';
}
