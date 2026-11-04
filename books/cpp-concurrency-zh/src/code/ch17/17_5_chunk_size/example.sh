#!/usr/bin/env bash
set -euo pipefail
# output: varies
# expect: chunk,ms
# tsan: no（计时示例，TSan 下太慢）
# 程序把总耗时写到 stdout，单块 p99 写到同目录的 p99.out，给第二张图读。
"$CXX" $CXXFLAGS 17_5_chunk_size.cpp -o "$BUILD/chunk_size"
"$BUILD/chunk_size"
