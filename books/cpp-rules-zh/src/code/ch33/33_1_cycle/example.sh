set -e

# 反例：两个头文件互相包含，先看到的那个类还没有对方的名字。
if $CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror main_bad.cpp -o "$BUILD/bad" 2> "$BUILD/bad.err"; then
    echo "expected the cyclic version to fail" >&2
    exit 1
fi
grep -q "error:" "$BUILD/bad.err"
echo "循环包含的版本：编译失败"

# 正例：各自前置声明对方，头文件之间不再互相包含。
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror main.cpp -o "$BUILD/app"
"$BUILD/app"
