// 第 12.4 节：按值返回的 Pop 有两个副作用，失败时值会丢
// mode: run
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Token {
    std::string id;
    static inline bool throw_on_assign = false;

    explicit Token(std::string value) : id(std::move(value)) {}
    Token(const Token&) = default;

    Token& operator=(const Token& other) {
        if (throw_on_assign) {
            throw std::runtime_error("token assignment failed");
        }
        id = other.id;
        return *this;
    }
};

template <class T>
class Stack {
public:
    void push(const T& value) { items_.push_back(value); }

    // 反例：弹出和交付是两个副作用，交付失败时元素已经没了
    T pop_value() {
        T result = items_.back();
        items_.pop_back();
        return result;
    }

    // 折中：先把值复制到调用方，成功后才修改栈
    void pop_into(T& result) {
        result = items_.back();
        items_.pop_back();
    }

    // 正例：查询和弹出各负一责，调用方自己决定什么时候复制
    T& top() { return items_.back(); }
    void pop() { items_.pop_back(); }

    std::size_t size() const noexcept { return items_.size(); }

private:
    std::vector<T> items_;
};

int main() {
    Stack<Token> stack;
    stack.push(Token{"alpha"});
    stack.push(Token{"beta"});

    Token received{"none"};
    Token::throw_on_assign = true;

    try {
        received = stack.pop_value();
    } catch (const std::exception& e) {
        std::printf("pop_value failed: %s\n", e.what());
    }
    std::printf("pop_value: stack=%zu received=[%s]\n", stack.size(),
                received.id.c_str());

    try {
        stack.pop_into(received);
    } catch (const std::exception& e) {
        std::printf("pop_into failed: %s\n", e.what());
    }
    std::printf("pop_into: stack=%zu received=[%s]\n", stack.size(),
                received.id.c_str());

    std::string delivered = stack.top().id;  // 复制成功后再弹出
    stack.pop();
    std::printf("top+pop: stack=%zu delivered=[%s]\n", stack.size(),
                delivered.c_str());
}
