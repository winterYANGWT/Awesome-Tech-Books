set -e
mkdir -p "$BUILD"

# C++ 实现 + C 调用方：两边共用同一个头文件，C++ 侧看到 extern "C"。
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -c sensor_api.cpp -o "$BUILD/sensor_api.o"
clang -std=c17 -Wall -Wextra -Wpedantic -Werror -c main.c -o "$BUILD/main.o"
$CXX "$BUILD/main.o" "$BUILD/sensor_api.o" -o "$BUILD/sensor_app"
"$BUILD/sensor_app"

if nm -g "$BUILD/sensor_api.o" | grep -q ' T sensor_format$'; then
    echo "sensor_api.o exports unmangled sensor_format"
fi

# 同一个函数名，如果没有 extern "C"，C++ 会导出改编后的符号。
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -c bad_link.cpp -o "$BUILD/bad_link.o"
clang -std=c17 -Wall -Wextra -Wpedantic -Werror -c probe.c -o "$BUILD/probe.o"
if nm -g "$BUILD/bad_link.o" | grep -q '_Z5probei'; then
    echo "bad_link.o exports mangled _Z5probei"
fi
if $CXX "$BUILD/probe.o" "$BUILD/bad_link.o" -o "$BUILD/probe_bad" 2> "$BUILD/link.err"; then
    echo "unexpected: link succeeded"
    exit 1
fi
echo "link failed as expected:"
grep -m1 -oE 'undefined (reference to|symbol:) .*' "$BUILD/link.err" || true
