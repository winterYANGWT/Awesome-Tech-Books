set -e

echo "先只把 main.cpp 交给编译器（还没有 BMI）："
if $CXX -std=c++20 -Wall -Wextra -Werror main.cpp -o "$BUILD/app_nobmi" 2>"$BUILD/nobmi.err"; then
    echo "  编译成功（不符合预期）"
    exit 1
fi
grep -m1 "module 'shop' not found" "$BUILD/nobmi.err" | sed 's/^/  /'

echo "先构建分区，再构建主接口，最后编译导入方："
$CXX -std=c++20 -Wall -Wextra -Werror --precompile pricing.cppm \
    -o "$BUILD/shop-pricing.pcm"
$CXX -std=c++20 -Wall -Wextra -Werror --precompile tax.cppm \
    -o "$BUILD/shop-tax.pcm"
$CXX -std=c++20 -Wall -Wextra -Werror --precompile shop.cppm \
    -fmodule-file=shop:pricing="$BUILD/shop-pricing.pcm" \
    -fmodule-file=shop:tax="$BUILD/shop-tax.pcm" \
    -o "$BUILD/shop.pcm"
$CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=shop="$BUILD/shop.pcm" \
    -fmodule-file=shop:pricing="$BUILD/shop-pricing.pcm" \
    -fmodule-file=shop:tax="$BUILD/shop-tax.pcm" \
    -c shop_impl.cpp -o "$BUILD/shop_impl.o"
$CXX -std=c++20 -Wall -Wextra -Werror \
    -fmodule-file=shop="$BUILD/shop.pcm" \
    -fmodule-file=shop:pricing="$BUILD/shop-pricing.pcm" \
    -fmodule-file=shop:tax="$BUILD/shop-tax.pcm" \
    main.cpp "$BUILD/shop.pcm" "$BUILD/shop-pricing.pcm" "$BUILD/shop-tax.pcm" \
    "$BUILD/shop_impl.o" -o "$BUILD/app"
"$BUILD/app"
