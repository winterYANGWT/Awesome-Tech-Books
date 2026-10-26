// 第 32.3 节：空循环体通常没有警告，空 if 体才会触发 -Wempty-body
// mode: warn
// expect: -Wempty-body
int main() {
    int processed = 0;

    // 这个分号让循环体变成空语句，clang 和 GCC 都不警告。
    for (int i = 0; i < 3; ++i);
        processed += 1;   // 缩进得像循环体，但它不属于循环

    // 同样的笔误放在 if 上，两个编译器都会警告。
    if (processed == 1);
    return processed;
}
