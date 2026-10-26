// 第 8.3 节：构造函数抛出时，只有构造完成的成员会被析构
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

class Slot;

struct SlotPool {
    static std::set<int> live;
    static int next_id;

    static int acquire(const std::string& name) {
        const int id = next_id++;
        live.insert(id);
        std::cout << "  获取槽位 " << id << "（" << name << "）\n";
        return id;
    }

    static void release(int id) {
        if (live.erase(id) != 0) {
            std::cout << "  释放槽位 " << id << "\n";
        }
    }
};

std::set<int> SlotPool::live;
int SlotPool::next_id = 1;

// 只为演示：真实代码里这个指针已经随构造失败而丢失。
Slot* g_leaked_image = nullptr;

class Slot {
public:
    explicit Slot(std::string name) : name_(std::move(name)) {
        if (name_ == "audio") {
            std::cout << "  创建 " << name_ << " 失败\n";
            throw std::runtime_error("audio 初始化失败");
        }
        id_ = SlotPool::acquire(name_);
    }

    ~Slot() { SlotPool::release(id_); }

    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;

private:
    std::string name_;
    int id_ = 0;
};

// 原始指针成员：构造函数体里先 new image，再 new audio。
// audio 抛出时 RawBook 没有构造完成，~RawBook 不会执行，image 泄漏。
class RawBook {
public:
    RawBook() {
        image_ = new Slot("image");
        g_leaked_image = image_;  // 演示用，便于程序结束时回收
        audio_ = new Slot("audio");
    }
    ~RawBook() {
        delete image_;
        delete audio_;
    }

private:
    Slot* image_ = nullptr;
    Slot* audio_ = nullptr;
};

// 在构造函数体里 catch、清理、再抛出：能补住，但清理代码要复制一份。
class CatchBook {
public:
    CatchBook() {
        try {
            image_ = new Slot("image");
            audio_ = new Slot("audio");
        } catch (...) {
            delete image_;
            image_ = nullptr;
            throw;
        }
    }
    ~CatchBook() {
        delete image_;
        delete audio_;
    }

private:
    Slot* image_ = nullptr;
    Slot* audio_ = nullptr;
};

// 把资源交给成员对象：image_ 构造完成后，audio_ 抛出会让 image_
// 作为“已构造完成的成员子对象”自动析构。
class SafeBook {
public:
    SafeBook()
        : image_(std::make_unique<Slot>("image")),
          audio_(std::make_unique<Slot>("audio")) {}

private:
    std::unique_ptr<Slot> image_;
    std::unique_ptr<Slot> audio_;
};

template <typename Book>
void try_construct(const char* label) {
    try {
        Book book;
    } catch (const std::exception& e) {
        std::cout << label << " 构造失败：" << e.what()
                  << "，仍占用 " << SlotPool::live.size() << " 个槽位\n";
    }
}

int main() {
    std::cout << "原始指针成员：\n";
    try_construct<RawBook>("RawBook");

    // 真实代码里这个指针已经丢了；演示程序靠全局指针把它找回来。
    delete g_leaked_image;
    g_leaked_image = nullptr;

    std::cout << "构造函数体内 catch：\n";
    try_construct<CatchBook>("CatchBook");

    std::cout << "成员对象：\n";
    try_construct<SafeBook>("SafeBook");
}
