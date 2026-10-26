set -e

CXX="${CXX:-clang++}"
BUILD="${BUILD:-build}"
mkdir -p "$BUILD"

echo "C++14，没有类外定义："
if "$CXX" -std=c++14 -Wall -Wextra -Wpedantic legacy_main.cpp \
        -o "$BUILD/legacy14_nodef" 2>"$BUILD/legacy14_nodef.err"; then
    echo "  链接成功（不符合预期）"
    exit 1
fi
grep -m1 'undefined reference to `Legacy::cap' "$BUILD/legacy14_nodef.err" | sed -n 's/.*\(undefined reference to .*\)/\1/p'

echo "C++14，补上 constexpr int Legacy::cap;："
"$CXX" -std=c++14 -Wall -Wextra -Wpedantic legacy_main.cpp legacy.cpp \
    -o "$BUILD/legacy14_def"
"$BUILD/legacy14_def"

echo "C++17，同一个头文件、不补类外定义："
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic legacy_main.cpp \
    -o "$BUILD/legacy17"
"$BUILD/legacy17"

echo "C++20，static const int 被取引用："
if "$CXX" -std=c++20 -Wall -Wextra -Wpedantic main.cpp \
        -o "$BUILD/budget_nodef" 2>"$BUILD/budget_nodef.err"; then
    echo "  链接成功（不符合预期）"
    exit 1
fi
grep -m1 'undefined reference to `Budget::limit' "$BUILD/budget_nodef.err" | sed -n 's/.*\(undefined reference to .*\)/\1/p'

echo "补上 const int Budget::limit; 后："
"$CXX" -std=c++20 -Wall -Wextra -Wpedantic main.cpp budget.cpp \
    -o "$BUILD/budget"
"$BUILD/budget"
