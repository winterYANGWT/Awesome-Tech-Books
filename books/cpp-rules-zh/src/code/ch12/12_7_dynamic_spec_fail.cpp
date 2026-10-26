// 第 12.7 节：C++17 起 throw(类型列表) 已从语言中移除
// mode: fail
// expect: dynamic exception specifications
void fetch() throw(int);

int main() {
}
