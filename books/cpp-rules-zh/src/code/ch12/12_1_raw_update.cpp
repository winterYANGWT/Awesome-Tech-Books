// 第 12.1 节：手工 delete/new 的更新连基本保证都没有
// mode: crash
// expect: revision=1
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>

struct Artwork {
    std::string title;

    explicit Artwork(std::string t) : title(std::move(t)) {
        // 模拟"画布申请失败"：构造到一半抛异常，对象从未存在
        if (title == "poison") {
            throw std::runtime_error("canvas unavailable");
        }
    }
};

class Gallery {
public:
    explicit Gallery(std::string title) : art_(new Artwork(std::move(title))) {}

    ~Gallery() { delete art_; }

    // 反例：先释放旧资源、再记账、最后才申请新资源
    void replace(const std::string& title) {
        delete art_;                 // 旧画已经没了
        ++revision_;                 // 账已经记上
        art_ = new Artwork(title);   // 这里抛出，art_ 仍指向已释放的对象
    }

    int revision() const noexcept { return revision_; }

private:
    Artwork* art_ = nullptr;
    int revision_ = 0;
};

int main() {
    Gallery gallery("dawn");

    try {
        gallery.replace("poison");
    } catch (const std::exception& e) {
        std::printf("update failed: %s\n", e.what());
    }

    std::printf("revision=%d\n", gallery.revision());
    // gallery 析构时会对悬垂指针再 delete 一次，ASan 在这里报错
}
