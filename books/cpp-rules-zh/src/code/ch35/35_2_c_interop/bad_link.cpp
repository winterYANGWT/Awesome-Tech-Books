// 第 35.2 节：没有 extern "C" 的 C++ 函数会以改编后的名字导出
int probe(int x) {
    return x + 1;
}
