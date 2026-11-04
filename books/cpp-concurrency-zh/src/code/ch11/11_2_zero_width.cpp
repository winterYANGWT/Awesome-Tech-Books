// mode: fail
// expect: zero width for bit-field
// 想用零宽位域分隔 bf1 和 bf3，但写成了具名形式；标准 C++ 只允许无名零宽位域。
struct Bits {
    unsigned bf1 : 3;
    unsigned bf2 : 0;
    unsigned bf3 : 3;
};

int main() {
    Bits bits{};
    return static_cast<int>(bits.bf1);
}
