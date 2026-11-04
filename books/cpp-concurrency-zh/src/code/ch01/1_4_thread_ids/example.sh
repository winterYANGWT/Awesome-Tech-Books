#!/usr/bin/env bash
# output: varies
# expect: ^== ps -L
set -euo pipefail

"$CXX" $CXXFLAGS 1_4_thread_ids.cpp -o "$BUILD/ids"
"$BUILD/ids" > "$BUILD/ids.out" &
pid=$!
sleep 0.4

echo "== /proc/$pid/task =="
ls "/proc/$pid/task" | tr '\n' ' '
echo
echo "== ps -L -p $pid =="
ps -L -p "$pid" -o pid,tid,comm

wait "$pid"
echo "== 程序自己的输出 =="
cat "$BUILD/ids.out"
