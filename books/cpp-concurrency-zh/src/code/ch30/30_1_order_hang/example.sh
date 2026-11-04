#!/usr/bin/env bash
# needs: mpi
# output: varies
# expect: 集合通信顺序不一致
# expect: 仍未结束
set -euo pipefail

mpicxx $CXXFLAGS 30_1_order_hang.cpp -o "$BUILD/order_hang"
set +e
timeout 10 $MPIRUN -np 4 "$BUILD/order_hang" > "$BUILD/run.log" 2>&1
rc=$?
set -e
sed "/^Abort is in progress/d" "$BUILD/run.log"
if [ "$rc" -eq 124 ]; then
    echo "10 秒后仍未结束：集合通信顺序不一致"
else
    echo "程序在 10 秒内退出（rc=$rc），没有复现挂起" >&2
    exit 1
fi
