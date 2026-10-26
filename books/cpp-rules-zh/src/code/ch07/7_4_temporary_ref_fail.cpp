// 第 7.4 节：临时对象不能绑定非 const 左值引用
// mode: fail
struct Reading {
    int value = 0;
    void bump() { ++value; }
};

Reading sample() {
    return Reading{1};
}

void take(Reading& reading) {
    reading.bump();
}

int main() {
    take(sample());        // 右值不能绑定 Reading&
    Reading& ref = sample();  // 同样不能绑定
    ref.bump();
}
