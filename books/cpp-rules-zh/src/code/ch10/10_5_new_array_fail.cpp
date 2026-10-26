// 第 10.5 节：new T[n] 要求 T 可默认构造
// mode: fail
// expect: Record
struct Record {
    explicit Record(int value) : value_(value) {}
    int value_;
};

int main() {
    Record* rows = new Record[3];   // 错误：Record 没有默认构造函数
    delete[] rows;
}
