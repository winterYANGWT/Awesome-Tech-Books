// 第 2.1 节：反例：换成强类型后，裸 int 的顺序错误无法编译
// mode: fail
// expect: no matching constructor
enum class Month { feb };

struct Day {
    explicit Day(int);
};

struct Year {
    explicit Year(int);
};

class Date {
public:
    Date(Month month, Day day, Year year);
};

int main() {
    Date d(2024, 2, 29);   // 2024 不是 Month，2 不是 Day，29 不是 Year
}
