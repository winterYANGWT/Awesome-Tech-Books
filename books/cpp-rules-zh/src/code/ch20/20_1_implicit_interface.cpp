// 第 20.1 节：模板按“合法表达式”检查类型，而不是按继承关系
#include <concepts>
#include <iostream>
#include <utility>

// size() 不必返回整数：这里返回一个能和 int 比较的类型
struct Celsius {
    double degrees;
};

bool operator>(Celsius lhs, int rhs) { return lhs.degrees > rhs; }

struct Gauge {
    int id;
    Celsius size() const { return {12.0}; }
    bool operator!=(const Gauge& other) const { return id != other.id; }
    void normalize() {
        std::cout << "Gauge::normalize\n";
        ++id;
    }
    void swap(Gauge& other) noexcept { std::swap(id, other.id); }
};

struct Ticket {
    int id;
    int size() const { return 3; }
    bool operator!=(const Ticket& other) const { return id != other.id; }
    void normalize() { std::cout << "Ticket::normalize\n"; }
    void swap(Ticket& other) noexcept { std::swap(id, other.id); }
};

// 接口不满足时，下面的 concept 为 false；不需要真的实例化 process
struct Unprepared {
    int size() const { return 1; }
    bool operator!=(const Unprepared&) const { return false; }
    void swap(Unprepared&) noexcept {}
};

template<class T>
concept Processable = requires(T& w, const T& other) {
    { w.size() > 10 } -> std::convertible_to<bool>;
    { w != other } -> std::convertible_to<bool>;
    w.normalize();
    w.swap(w);
    T(w);
};

static_assert(Processable<Gauge>);
static_assert(Processable<Ticket>);
static_assert(!Processable<Unprepared>);  // 少一个 normalize

template<class T>
void process(T& w, const T& other) {
    if (w.size() > 10 && w != other) {
        T temp(w);
        temp.normalize();
        temp.swap(w);
    }
}

int main() {
    Gauge g{1}, other{2};
    process(g, other);
    std::cout << "Gauge id after swap = " << g.id << '\n';

    Ticket t{7}, t2{8};
    process(t, t2);  // size() 是 3，条件不成立，什么也不做
    std::cout << "Ticket id = " << t.id << '\n';
}
