#!/usr/bin/env bash
# 演示：编译器检查通过，clang-tidy 的静态检查仍能发现模式问题
set -euo pipefail
cd "$(dirname "$0")"

$CXX -std=c++20 -Wall -Wextra -Wpedantic -Werror -fsyntax-only sample.cpp
echo "compiler warnings: none"

clang-tidy sample.cpp \
    --checks='-*,bugprone-use-after-move,modernize-use-nullptr,readability-container-size-empty' \
    -- -std=c++20 > "$BUILD/tidy.log" 2>&1

for check in bugprone-use-after-move modernize-use-nullptr readability-container-size-empty; do
    if ! grep -q "\[$check\]" "$BUILD/tidy.log"; then
        echo "clang-tidy did not report $check"
        cat "$BUILD/tidy.log"
        exit 1
    fi
done

echo "clang-tidy findings:"
grep -E 'warning: .*\[(bugprone-use-after-move|modernize-use-nullptr|readability-container-size-empty)\]' \
    "$BUILD/tidy.log" | sed -E 's#^.*/sample\.cpp#sample.cpp#'
