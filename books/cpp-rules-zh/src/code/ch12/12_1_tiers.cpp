// 第 12.1 节：基本、强、不抛三档保证在实际状态上的差别
// mode: run
#include <cstdio>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct Letter {
    std::string body;

    explicit Letter(std::string b) : body(std::move(b)) {}

    Letter(const Letter& other) : body(other.body) {
        // 只有"poison"这封信在复制时失败，用来精确控制异常发生的位置
        if (body == "poison") {
            throw std::runtime_error("cannot copy a poison letter");
        }
    }

    Letter& operator=(const Letter&) = default;
};

class Mailbox {
public:
    Mailbox() = default;

    // 基本保证：已经投递的信留着，后面的没投进去，对象仍然有效
    void deliver_basic(const std::vector<Letter>& batch) {
        for (const Letter& letter : batch) {
            letters_.push_back(letter);
        }
    }

    // 强保证：先在副本上完成全部投递，成功后才提交
    void deliver_strong(const std::vector<Letter>& batch) {
        Mailbox temp(*this);       // 副本构造可能抛，原对象不受影响
        temp.deliver_basic(batch); // 中途抛，temp 析构，原对象不受影响
        swap(temp);                // 提交阶段只做不抛的交换
    }

    void swap(Mailbox& other) noexcept { letters_.swap(other.letters_); }

    std::size_t size() const noexcept { return letters_.size(); }

private:
    std::vector<Letter> letters_;
};

void swap(Mailbox& a, Mailbox& b) noexcept { a.swap(b); }

static_assert(std::is_nothrow_destructible_v<Mailbox>);
static_assert(std::is_nothrow_swappable_v<Mailbox>);

int main() {
    Mailbox mailbox;
    std::vector<Letter> seed;
    seed.emplace_back("welcome");
    mailbox.deliver_basic(seed);

    std::vector<Letter> batch;
    batch.emplace_back("alpha");
    batch.emplace_back("beta");
    batch.emplace_back("poison");
    batch.emplace_back("gamma");

    try {
        mailbox.deliver_strong(batch);
    } catch (const std::exception& e) {
        std::printf("strong failed: %s\n", e.what());
    }
    std::printf("after strong failure: size=%zu\n", mailbox.size());

    try {
        mailbox.deliver_basic(batch);
    } catch (const std::exception& e) {
        std::printf("basic failed: %s\n", e.what());
    }
    std::printf("after basic failure: size=%zu\n", mailbox.size());

    std::printf("nothrow: size()=%zu, destructor and swap are noexcept\n",
                mailbox.size());
}
