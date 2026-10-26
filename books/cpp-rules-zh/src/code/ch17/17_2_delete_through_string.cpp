// 第 17.2 节：从没有虚析构的具体类派生，再经基类指针删除
// mode: crash
// expect: AddressSanitizer
#include <iostream>
#include <string>

// 标准库的 std::string 不是为当基类设计的：没有虚析构，也没有 protected 成员。
class TaggedText : public std::string {
public:
    explicit TaggedText(const char* text) : std::string(text) {}
    std::size_t tag() const { return tag_; }

private:
    std::size_t tag_ = 42;
};

int main() {
    auto* tagged = new TaggedText("hello");
    std::cout << "文本=" << static_cast<const std::string&>(*tagged)
              << " 标记=" << tagged->tag() << '\n';

    std::string* base = tagged;
    delete base;  // 未定义行为：只调用 std::string 的析构，按错误的大小释放
    std::cout << "如果这行还能执行，说明问题被静默吞掉了\n";
}
