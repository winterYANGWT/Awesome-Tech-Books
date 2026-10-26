set -e

$CXX -std=c++20 -Wall -Wextra -Werror --precompile catalog.cppm -o "$BUILD/catalog.pcm"
$CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=catalog="$BUILD/catalog.pcm" \
    -c catalog.cpp -o "$BUILD/catalog.o"
$CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=catalog="$BUILD/catalog.pcm" \
    main.cpp "$BUILD/catalog.pcm" "$BUILD/catalog.o" -o "$BUILD/app"
"$BUILD/app"

echo "换用没有显式实例化的 long long："
if $CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=catalog="$BUILD/catalog.pcm" \
        main_long.cpp "$BUILD/catalog.pcm" "$BUILD/catalog.o" \
        -o "$BUILD/app_long" 2>"$BUILD/long.err"; then
    echo "  链接成功（不符合预期）"
    exit 1
fi
if grep -q 'discounted.*long long' "$BUILD/long.err"; then
    echo "  链接失败：找不到 store::discounted<long long> 的定义"
else
    echo "  链接失败，但错误里没有 discounted<long long>" >&2
    cat "$BUILD/long.err" >&2
    exit 1
fi
