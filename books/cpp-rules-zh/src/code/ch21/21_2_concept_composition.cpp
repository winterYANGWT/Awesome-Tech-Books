// 第 21.2 节：定义概念、用 && 和 || 组合，并观察更严格的约束优先
#include <concepts>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// 一个概念就是一个编译期布尔谓词：这些表达式必须都能编译。
template <typename T>
concept Printable = requires(std::ostream& os, const T& value) {
    os << value;
};

template <typename T>
concept Named = requires(const T& value) {
    { value.name() } -> std::convertible_to<std::string>;
};

// && 把两个概念合成一个更严格的概念。
template <typename T>
concept Reportable = Printable<T> && Named<T>;

// requires 表达式里可以检查成员函数返回类型，也可以检查嵌套类型是否存在。
template <typename T>
concept Sized = requires(const T& value) {
    { value.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept Associative = Sized<T> && requires { typename T::key_type; };

// || 表示“满足任意一个即可”。
template <typename T>
concept StringLike = std::same_as<T, std::string> || std::same_as<T, std::string_view>;

struct Point {
    int x;
    int y;
};

std::ostream& operator<<(std::ostream& os, const Point& point) {
    return os << '(' << point.x << ", " << point.y << ')';
}

struct User {
    std::string id;

    std::string name() const { return id; }
};

std::ostream& operator<<(std::ostream& os, const User& user) {
    return os << user.id;
}

// 两个重载都可行时，约束更强（包含另一个约束）的那个胜出。
template <Printable T>
void report(const T& value) {
    std::cout << "只可打印: " << value << '\n';
}

template <Reportable T>
void report(const T& value) {
    std::cout << "可报告: " << value.name() << '\n';
}

template <StringLike T>
void echo(T text) {
    std::cout << "字符串: " << text << '\n';
}

// std::string 同时满足 StringLike 和更严格的 same_as<std::string>，后者胜出。
template <std::same_as<std::string> T>
void echo(T text) {
    std::cout << "精确 string: " << text << '\n';
}

int main() {
    report(Point{3, 4});       // Point 只满足 Printable
    report(User{"u-7"});       // User 同时满足 Printable 和 Named，选 Reportable 重载

    echo(std::string("alpha"));
    echo(std::string_view("beta"));

    static_assert(Printable<Point>);
    static_assert(!Named<Point>);
    static_assert(Reportable<User>);
    static_assert(!Reportable<Point>);

    static_assert(Sized<std::string>);
    static_assert(Sized<std::vector<int>>);
    static_assert(Associative<std::map<int, int>>);
    static_assert(!Associative<std::vector<int>>);

    static_assert(StringLike<std::string>);
    static_assert(StringLike<std::string_view>);
    static_assert(!StringLike<const char*>);
}
