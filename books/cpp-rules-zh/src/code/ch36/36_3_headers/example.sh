#!/usr/bin/env bash
# 演示：头文件单独编译时必须自足；不自足的头文件在换一个包含顺序后就会坏
set -euo pipefail
cd "$(dirname "$0")"

if $CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only -x c++-header audit.hpp \
        > "$BUILD/audit.log" 2>&1; then
    echo "unexpected: audit.hpp compiled on its own"
    exit 1
fi
echo "audit.hpp compiled alone: failed as expected"
grep -m 3 'error:' "$BUILD/audit.log" | sed 's/^/  /'

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only -x c++-header fixed_audit.hpp
echo "fixed_audit.hpp compiled alone: ok"

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror main.cpp -o "$BUILD/audit_main"
"$BUILD/audit_main"
