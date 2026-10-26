set -e
mkdir -p "$BUILD"

# libstdc++ 的新旧 ABI 可以在同一台机器上共存，string 的布局不同。
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    -D_GLIBCXX_USE_CXX11_ABI=1 probe.cpp -o "$BUILD/probe_new"
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    -D_GLIBCXX_USE_CXX11_ABI=0 probe.cpp -o "$BUILD/probe_old"

new_layout=$("$BUILD/probe_new")
old_layout=$("$BUILD/probe_old")
echo "new ABI: $new_layout"
echo "old ABI: $old_layout"

if [ "$new_layout" = "$old_layout" ]; then
    echo "expected the two layouts to differ"
    exit 1
fi
