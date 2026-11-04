#!/usr/bin/env bash
# mode: run
# timeout: 30
set -euo pipefail

"$CXX" $CXXFLAGS main.cpp -o "$BUILD/thread_ids"

rm -f "$BUILD/pid"
"$BUILD/thread_ids" "$BUILD/pid" > "$BUILD/program.out" &
runner=$!

for _ in $(seq 1 100); do
    if [ -s "$BUILD/pid" ]; then
        break
    fi
    sleep 0.05
done

pid=$(cat "$BUILD/pid")

echo "ps -L -p $pid（同一 PID 三行）："
ps -L -p "$pid" -o pid=,tid=,comm=
echo "ps -L 的线程数：$(ps -L -p "$pid" -o tid= | wc -l)"

echo "/proc/$pid/task 下的目录项："
ls -1 "/proc/$pid/task" | tr '\n' ' '
echo
echo "/proc/$pid/task 的目录项数：$(ls -1 "/proc/$pid/task" | wc -l)"

wait "$runner"
echo "程序输出："
cat "$BUILD/program.out"
