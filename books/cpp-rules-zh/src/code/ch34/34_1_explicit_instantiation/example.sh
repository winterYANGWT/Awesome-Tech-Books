set -e

$CXX -std=c++20 -Wall -Wextra -Werror -I. -c quote.cpp -o "$BUILD/quote.o"
$CXX -std=c++20 -Wall -Wextra -Werror -I. -c main.cpp -o "$BUILD/main.o"
$CXX "$BUILD/main.o" "$BUILD/quote.o" -o "$BUILD/app"
"$BUILD/app"

echo "quote.o 里 clamp_charge<int> 的符号数：$(nm -C "$BUILD/quote.o" | grep -c 'clamp_charge<int>' || true)"
echo "quote.o 里 clamp_charge<double> 的符号数：$(nm -C "$BUILD/quote.o" | grep -c 'clamp_charge<double>' || true)"
echo "main.o 对 int 版的引用："
nm -C "$BUILD/main.o" | grep ' U .*clamp_charge<int>' | sed 's/^/  /'

echo "换用未显式实例化的 double："
if $CXX -std=c++20 -Wall -Wextra -Werror -I. main_other.cpp "$BUILD/quote.o" \
        -o "$BUILD/app_other" 2>"$BUILD/other.err"; then
    echo "  链接成功（不符合预期）"
    exit 1
fi
if grep -q 'clamp_charge<double>' "$BUILD/other.err"; then
    echo "  链接失败：undefined reference to shipping::clamp_charge<double>(double)"
else
    echo "  链接失败，但错误里没有 clamp_charge<double>" >&2
    cat "$BUILD/other.err" >&2
    exit 1
fi
