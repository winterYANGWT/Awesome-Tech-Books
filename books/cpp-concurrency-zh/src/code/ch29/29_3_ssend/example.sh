#!/usr/bin/env bash
# needs: mpi
# expect: safe：4 个 rank 完成环交换
set -euo pipefail

mpicc $CFLAGS ssend.c -o "$BUILD/ssend"

echo "-- 先跑不加区分的 Ssend --"
set +e
timeout --foreground 5 $MPIRUN -np 4 "$BUILD/ssend" > "$BUILD/naive.log" 2>&1
rc=$?
set -e
cat "$BUILD/naive.log"
if [ "$rc" -ne 124 ]; then
    echo "预期超时，实际退出码 $rc"
    exit 1
fi
echo "naive Ssend：超时，deadlock detected"

echo "-- 再跑偶数先发、奇数先收 --"
$MPIRUN -np 4 "$BUILD/ssend" --safe
