set -e

$CXX -std=c++20 -Wall -Wextra -Werror -I. client.cpp report_pimpl.cpp -o "$BUILD/app"
"$BUILD/app"

# -H 把头文件包含树打到 stderr；只数 legacy_formatter.h 出现与否。
count_inherit=$($CXX -std=c++20 -I. -H -fsyntax-only inherit_client.cpp 2>&1 | grep -c 'legacy_formatter.h' || true)
count_pimpl=$($CXX -std=c++20 -I. -H -fsyntax-only client.cpp 2>&1 | grep -c 'legacy_formatter.h' || true)

if [ "$count_inherit" -eq 0 ]; then
    echo "expected inherit_client.cpp to include legacy_formatter.h" >&2
    exit 1
fi
if [ "$count_pimpl" -ne 0 ]; then
    echo "client.cpp should not include legacy_formatter.h" >&2
    exit 1
fi

echo "继承版客户端头文件链包含 legacy_formatter.h：是"
echo "Pimpl 版客户端头文件链包含 legacy_formatter.h：否"
