// 第 18.1 节：委托版赋值能把工作放在一旁完成，私有继承版被基类的赋值锁住
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class LegacyBuffer {
public:
    explicit LegacyBuffer(std::array<int, 3> data) : data_(data) {}

    LegacyBuffer(const LegacyBuffer& other) { copyFrom(other); }

    LegacyBuffer& operator=(const LegacyBuffer& other) {
        if (this != &other) {
            copyFrom(other);  // 可能写到一半就抛
        }
        return *this;
    }

    const std::array<int, 3>& data() const { return data_; }

private:
    void copyFrom(const LegacyBuffer& other) {
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (other.data_[i] == 8) {
                throw std::runtime_error("copy failed at element " + std::to_string(i));
            }
            data_[i] = other.data_[i];
        }
    }

    std::array<int, 3> data_{};
};

// 私有继承：隐式生成的赋值运算符会调用 LegacyBuffer::operator=。
class InheritedDoc : private LegacyBuffer {
public:
    using LegacyBuffer::LegacyBuffer;
    using LegacyBuffer::data;
};

// 委托：先把新值完整复制到旁边，成功后再替换指针。
class DelegatingDoc {
public:
    explicit DelegatingDoc(std::array<int, 3> data)
        : rep_(std::make_unique<LegacyBuffer>(data)) {}

    DelegatingDoc(const DelegatingDoc& other)
        : rep_(std::make_unique<LegacyBuffer>(*other.rep_)) {}

    DelegatingDoc& operator=(const DelegatingDoc& other) {
        auto fresh = std::make_unique<LegacyBuffer>(*other.rep_);  // 失败时原对象不变
        rep_ = std::move(fresh);                                   // 只做指针赋值
        return *this;
    }

    const std::array<int, 3>& data() const { return rep_->data(); }

private:
    std::unique_ptr<LegacyBuffer> rep_;
};

template <class Doc>
void printData(const char* label, const Doc& doc) {
    std::cout << label;
    for (int value : doc.data()) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';
}

int main() {
    InheritedDoc inheritedTarget(std::array<int, 3>{10, 20, 30});
    InheritedDoc inheritedSource(std::array<int, 3>{7, 8, 9});
    try {
        inheritedTarget = inheritedSource;
    } catch (const std::runtime_error& error) {
        std::cout << "继承版捕获：" << error.what() << '\n';
    }
    printData("继承版赋值后：", inheritedTarget);

    DelegatingDoc delegatedTarget(std::array<int, 3>{10, 20, 30});
    DelegatingDoc delegatedSource(std::array<int, 3>{7, 8, 9});
    try {
        delegatedTarget = delegatedSource;
    } catch (const std::runtime_error& error) {
        std::cout << "委托版捕获：" << error.what() << '\n';
    }
    printData("委托版赋值后：", delegatedTarget);
}
