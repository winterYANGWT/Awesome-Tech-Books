// 第 2.4 节：反例：一个类型同时向两个方向转换，重载决议二义
// mode: fail
// expect: ambiguous
struct Orange {};
struct Pear {};

struct Apple {
    operator Orange() const { return {}; }
    operator Pear() const { return {}; }
};

void eat(Orange) {}
void eat(Pear) {}

int main() {
    Apple a;
    eat(a);   // Apple -> Orange 还是 Apple -> Pear？
}
