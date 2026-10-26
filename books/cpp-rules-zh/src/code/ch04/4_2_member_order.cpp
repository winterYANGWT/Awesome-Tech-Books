// 第 4.2 节：基类和成员的实际构造顺序由声明顺序决定
#include <iostream>
#include <string>
#include <utility>

void note(const std::string& what) {
    std::cout << "  " << what << "\n";
}

struct Clock {
    Clock() { note("基类 Clock"); }
};

struct Sink {
    Sink() { note("基类 Sink"); }
};

struct Buffer {
    std::string name;

    explicit Buffer(std::string n) : name(std::move(n)) {
        note("成员 " + name);
    }
    ~Buffer() { note("析构成员 " + name); }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
};

// Sink 在基类列表里写在前面，所以先于 Clock 构造
class Channel : public Sink, public Clock {
public:
    Channel() : Sink(), Clock(), first_("first_"), second_("second_") {
        note("Channel 的构造函数体");
    }

private:
    Buffer first_;
    Buffer second_;
};

// 声明顺序保证 full_name_ 用到 first_、last_ 时，它们已经构造完成
class NamePlate {
public:
    NamePlate(std::string first, std::string last)
        : first_(std::move(first)), last_(std::move(last)),
          full_name_(first_ + " " + last_) {}

    const std::string& full_name() const { return full_name_; }

private:
    std::string first_;
    std::string last_;
    std::string full_name_;
};

int main() {
    std::cout << "--- Channel ---\n";
    {
        Channel channel;
    }

    std::cout << "--- NamePlate ---\n";
    NamePlate plate("Ada", "Lovelace");
    std::cout << "  full_name_ = " << plate.full_name() << "\n";
}
