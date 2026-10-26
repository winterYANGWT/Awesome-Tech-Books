// 第 36.4 节：const 成员函数里 find 返回 const_iterator，赋给 iterator 会触发长模板报错
// mode: fail
// expect: const_iterator
#include <iostream>
#include <map>
#include <string>

class Directory {
public:
    void show_email(const std::string& nickname) const {
        // entries_ 在 const 成员函数里是 const map，find 返回 const_iterator
        std::map<std::string, std::string>::iterator it = entries_.find(nickname);
        if (it != entries_.end()) {
            std::cout << nickname << " -> " << it->second << '\n';
        }
    }

private:
    std::map<std::string, std::string> entries_;
};
