set -e

# 反例：析构函数在头文件里 = default，客户端销毁 Session 时 Impl 仍然不完整。
if $CXX -std=c++20 -Wall -Wextra -Wpedantic bad_main.cpp bad_session.cpp \
        -o "$BUILD/bad" 2> "$BUILD/bad.err"; then
    echo "expected the bad Session to fail" >&2
    exit 1
fi
grep -q "incomplete type" "$BUILD/bad.err"
echo "析构函数定义在头文件里：编译失败（incomplete type）"
