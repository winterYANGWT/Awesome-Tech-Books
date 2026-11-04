#!/usr/bin/env bash
# needs: mpi
# mode: hang
# timeout: 12
# expect: 仍未结束
set -euo pipefail

mpicc $CFLAGS wrong_tag.c -o "$BUILD/wrong_tag"

set +e
timeout --foreground 5 $MPIRUN -np 2 "$BUILD/wrong_tag" > "$BUILD/prog.log" 2>&1
rc=$?
set -e

cat "$BUILD/prog.log"
if [ "$rc" -eq 124 ]; then
    echo "（5 秒后仍未结束，强制终止）"
else
    echo "预期超时，实际退出码 $rc"
    exit 1
fi
