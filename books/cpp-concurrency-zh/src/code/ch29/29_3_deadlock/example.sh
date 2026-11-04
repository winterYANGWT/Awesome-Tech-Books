#!/usr/bin/env bash
# needs: mpi
# mode: hang
# timeout: 16
# expect: 仍未结束
set -euo pipefail

mpicc $CFLAGS deadlock.c -o "$BUILD/deadlock"

set +e
timeout --foreground 10 $MPIRUN -np 4 "$BUILD/deadlock" > "$BUILD/prog.log" 2>&1
rc=$?
set -e

cat "$BUILD/prog.log"
if [ "$rc" -eq 124 ]; then
    echo "（10 秒后仍未结束，强制终止）"
else
    echo "预期超时，实际退出码 $rc"
    exit 1
fi
