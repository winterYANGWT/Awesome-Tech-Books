set -e

$CXX -std=c++20 -Wall -Wextra -Werror --precompile geometry.cppm -o "$BUILD/geometry.pcm"
$CXX -std=c++20 -Wall -Wextra -Werror -fmodule-file=geometry="$BUILD/geometry.pcm" \
    main.cpp "$BUILD/geometry.pcm" -o "$BUILD/app"
"$BUILD/app"

echo "模块内部函数 square 在导入方："
if $CXX -std=c++20 -fmodule-file=geometry="$BUILD/geometry.pcm" \
        hidden.cpp "$BUILD/geometry.pcm" -o "$BUILD/hidden" 2>"$BUILD/hidden.err"; then
    echo "  可见（不符合预期）"
    exit 1
fi
grep -m1 "no member named 'square' in namespace 'geo'" "$BUILD/hidden.err" | sed 's/^/  /'

echo "模块接口里的宏 GEOMETRY_INTERNAL_LIMIT 在导入方："
if $CXX -std=c++20 -fmodule-file=geometry="$BUILD/geometry.pcm" \
        macro.cpp "$BUILD/geometry.pcm" -o "$BUILD/macro" 2>"$BUILD/macro.err"; then
    echo "  不可见"
else
    echo "  泄漏了（不符合预期）"
    cat "$BUILD/macro.err" >&2
    exit 1
fi
