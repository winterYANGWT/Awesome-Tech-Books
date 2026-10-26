#!/usr/bin/env bash
# 同一个源文件编译两次：一次开断言，一次定义 NDEBUG
set -euo pipefail
cd "$(dirname "$0")"

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror side_effect.cpp -o "$BUILD/debug"
$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -DNDEBUG side_effect.cpp -o "$BUILD/release"

printf 'assert enabled: '
"$BUILD/debug"
printf 'NDEBUG defined: '
"$BUILD/release"
