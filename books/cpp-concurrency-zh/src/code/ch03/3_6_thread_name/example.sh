#!/usr/bin/env bash
# mode: run
# timeout: 30
set -euo pipefail

"$CC" $CFLAGS main.c -o "$BUILD/thread_name"

rm -f "$BUILD/ready"
"$BUILD/thread_name" "$BUILD/ready" > "$BUILD/program.out" &
pid=$!

for _ in $(seq 1 100); do
    if [ -s "$BUILD/ready" ]; then
        break
    fi
    sleep 0.05
done

echo "ps -L 的 comm 统计（每个名字一行计数）："
ps -L -p "$pid" -o comm= | sort | uniq -c

echo "/proc/$pid/task/*/comm 的统计："
cat /proc/"$pid"/task/*/comm | sort | uniq -c

wait "$pid"
cat "$BUILD/program.out"
