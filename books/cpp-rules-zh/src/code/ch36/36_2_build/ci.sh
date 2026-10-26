#!/usr/bin/env bash
# 一条命令：配置、构建、跑测试。CI 服务器执行的就是这个文件。
set -euo pipefail
cd "$(dirname "$0")"
build="${1:-build}"

cmake -S . -B "$build" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build" --parallel 2
ctest --test-dir "$build" --output-on-failure
