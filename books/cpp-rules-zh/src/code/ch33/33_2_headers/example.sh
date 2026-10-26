set -e

# 每个公开头文件都单独编译一次，模拟 CI 里的“header self-sufficiency”检查。
for header in money.h account.h report.h; do
    $CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only -x c++-header "$header"
    echo "$header: 单独编译通过"
done

# 缺少 <string> 的头文件必须在这里被抓住，而不是等某个客户端碰巧先包含了 <string>。
if $CXX -std=c++20 -Wall -Wextra -Wpedantic -fsyntax-only -x c++-header missing_include.h \
        2> "$BUILD/missing.err"; then
    echo "expected missing_include.h to fail" >&2
    exit 1
fi
grep -q "string" "$BUILD/missing.err"
echo "missing_include.h: 缺少 <string>，单独编译失败（符合预期）"

# 正常客户端：头文件自足，守卫挡住重复包含，ADL 找到 operator<<。
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror account.cpp report.cpp main.cpp -o "$BUILD/app"
"$BUILD/app"

# 把带外部链接的变量定义放进头文件，两个翻译单元各定义一份，链接器会拒绝。
if $CXX -std=c++20 -Wall -Wextra -Wpedantic counter_a.cpp counter_b.cpp counter_main.cpp \
        -o "$BUILD/dup" 2> "$BUILD/dup.err"; then
    echo "expected a duplicate definition" >&2
    exit 1
fi
grep -q "multiple definition" "$BUILD/dup.err"
echo "shared_counter.h 被两个翻译单元包含：链接时重复定义（符合预期）"
