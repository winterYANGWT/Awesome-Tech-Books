// 第 2.1 节：用 Day/Month/Year 把参数顺序错误挡在编译期
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

// Month 只有 12 个合法值，用 enum class 后连 int 都不能隐式传进来。
enum class Month : unsigned char {
    jan = 1, feb, mar, apr, may, jun,
    jul, aug, sep, oct, nov, dec
};

class Day {
public:
    explicit Day(int d) : value_(d) {
        if (d < 1 || d > 31) throw std::invalid_argument("day outside 1..31");
    }
    int value() const { return value_; }
private:
    int value_;
};

class Year {
public:
    explicit Year(int y) : value_(y) {
        if (y < 1900 || y > 2200) throw std::invalid_argument("year outside 1900..2200");
    }
    int value() const { return value_; }
private:
    int value_;
};

// 类型设计的一部分：构造时就检查“这个月到底有没有这一天”。
class Date {
public:
    Date(Month m, Day d, Year y) : month_(m), day_(d.value()), year_(y.value()) {
        if (day_ > days_in_month()) {
            throw std::invalid_argument("day does not exist in this month");
        }
    }

    std::string to_string() const {
        return std::to_string(year_) + "-" + two(static_cast<int>(month_)) + "-" + two(day_);
    }

private:
    static bool leap(int y) {
        return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    }

    int days_in_month() const {
        switch (month_) {
        case Month::feb: return leap(year_) ? 29 : 28;
        case Month::apr: case Month::jun:
        case Month::sep: case Month::nov: return 30;
        default: return 31;
        }
    }

    static std::string two(int n) {
        return n < 10 ? "0" + std::to_string(n) : std::to_string(n);
    }

    Month month_;
    int day_;
    int year_;
};

int main() {
    // 编译期就能证明 Month 不会悄悄变成 int。
    static_assert(!std::is_convertible_v<Month, int>);

    Date ok(Month::feb, Day(29), Year(2024));
    std::cout << ok.to_string() << " 合法\n";

    try {
        Date bad(Month::feb, Day(29), Year(2023));
        std::cout << bad.to_string() << " 不应到这里\n";
    } catch (const std::invalid_argument& e) {
        // 顺序错了编译不过；2 月 29 日是否存在只能运行期判断。
        std::cout << "2023-02-29 被拒绝：" << e.what() << '\n';
    }
}
