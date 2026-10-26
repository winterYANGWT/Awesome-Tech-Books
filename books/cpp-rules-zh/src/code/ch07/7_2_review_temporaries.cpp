// 第 7.2 节：代码评审先找值传参、重复求 end、后置自增和隐式转换
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct Label {
    static int copies;
    static int conversions;
    std::string text;

    Label() = default;
    Label(const char* value) : text(value) { ++conversions; }  // 故意不写 explicit
    Label(std::string value) : text(std::move(value)) { ++conversions; }
    Label(const Label& other) : text(other.text) { ++copies; }
    Label(Label&&) noexcept = default;
    Label& operator=(const Label&) = default;
    Label& operator=(Label&&) noexcept = default;
};
int Label::copies = 0;
int Label::conversions = 0;

struct Employee {
    static int copies;
    Label name;
    Label address;

    Employee(Label n, Label a) : name(std::move(n)), address(std::move(a)) {}
    Employee(const Employee& other)
        : name(other.name), address(other.address) {
        ++copies;
    }
};
int Employee::copies = 0;

struct Iterator {
    using Inner = std::vector<Employee>::const_iterator;

    static int postfix_calls;
    Inner current;

    explicit Iterator(Inner it) : current(it) {}
    Iterator(const Iterator& other) : current(other.current) {}

    Iterator& operator++() {
        ++current;
        return *this;
    }

    Iterator operator++(int) {
        ++postfix_calls;
        Iterator old(*this);
        ++current;
        return old;
    }

    const Employee& operator*() const { return *current; }
    bool operator!=(const Iterator& other) const { return current != other.current; }
};
int Iterator::postfix_calls = 0;

struct Directory {
    static int end_calls;
    std::vector<Employee> people;

    explicit Directory(std::vector<Employee> entries) : people(std::move(entries)) {}

    Iterator begin() const { return Iterator(people.begin()); }
    Iterator end() const {
        ++end_calls;
        return Iterator(people.end());
    }
};
int Directory::end_calls = 0;

bool same(const Label& lhs, const Label& rhs) {
    return lhs.text == rhs.text;
}

std::string find_bad(Directory directory, Label wanted) {
    for (Iterator i = directory.begin(); i != directory.end(); i++) {
        if (same((*i).name, wanted)) {
            return (*i).address.text;
        }
    }
    return {};
}

std::string find_good(const Directory& directory, const Label& wanted) {
    Iterator stop = directory.end();
    for (Iterator i = directory.begin(); i != stop; ++i) {
        if (same((*i).name, wanted)) {
            return (*i).address.text;
        }
    }
    return {};
}

void reset_counts() {
    Label::copies = 0;
    Label::conversions = 0;
    Employee::copies = 0;
    Iterator::postfix_calls = 0;
    Directory::end_calls = 0;
}

int main() {
    Directory directory(std::vector<Employee>{
        Employee(Label("Ada"), Label("A-101")),
        Employee(Label("Grace"), Label("B-204")),
        Employee(Label("Linus"), Label("C-307")),
    });

    Label wanted("Grace");
    Label missing("Nobody");

    reset_counts();
    std::string bad = find_bad(directory, missing);
    std::cout << "按值传参：Employee 拷贝 " << Employee::copies
              << " 次，Label 拷贝 " << Label::copies
              << " 次，end() 调用 " << Directory::end_calls
              << " 次，后置自增 " << Iterator::postfix_calls << " 次\n";

    reset_counts();
    std::string good = find_good(directory, wanted);
    std::cout << "const& 传参：Employee 拷贝 " << Employee::copies
              << " 次，Label 拷贝 " << Label::copies
              << " 次，end() 调用 " << Directory::end_calls
              << " 次，后置自增 " << Iterator::postfix_calls << " 次\n";

    reset_counts();
    bool converted = same(directory.people[0].name, "Ada");
    std::cout << "隐式转换：same(name, \"Ada\") 构造临时 Label "
              << Label::conversions << " 次，结果 " << converted << "\n";

    reset_counts();
    bool direct = directory.people[0].name.text == "Ada";
    std::cout << "直接比较成员：构造临时 Label " << Label::conversions
              << " 次，结果 " << direct << "\n";

    std::cout << "查到 Grace 的地址：" << good
              << "，查 Nobody 的结果：" << (bad.empty() ? "未找到" : bad) << "\n";
}
