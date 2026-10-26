// 第 36.4 节：用别名缩短类型名、用 auto 让编译器给出正确的迭代器类型
// mode: run
#include <iostream>
#include <map>
#include <string>
#include <utility>

class Directory {
public:
    void add(std::string nickname, std::string email) {
        entries_.insert_or_assign(std::move(nickname), std::move(email));
    }

    void show_email(const std::string& nickname) const {
        // NicknameMap 是短名字；auto 直接得到 const_iterator，不需要手写类型
        const auto it = entries_.find(nickname);
        if (it == entries_.end()) {
            std::cout << nickname << ": not found\n";
            return;
        }
        std::cout << nickname << " -> " << it->second << '\n';
    }

private:
    using NicknameMap = std::map<std::string, std::string>;
    NicknameMap entries_;
};

int main() {
    Directory directory;
    directory.add("ada", "ada@example.org");
    directory.add("grace", "grace@example.org");
    directory.show_email("ada");
    directory.show_email("linus");
}
