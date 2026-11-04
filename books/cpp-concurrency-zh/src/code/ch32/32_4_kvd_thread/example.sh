#!/usr/bin/env bash
# mode: run
# timeout: 40
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CC" $CFLAGS hold_client.c -o "$BUILD/hold"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

portfile="$BUILD/kvd.port"
rm -f "$portfile"
"$BUILD/server" --port-file "$portfile" > "$BUILD/server.log" 2>&1 &
srv=$!
trap 'kill "$srv" 2>/dev/null || true; wait "$srv" 2>/dev/null || true; rm -f "$portfile"' EXIT

for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

# 5 个连接各占一个线程，其中 A 保持 3 秒。
hold_pids=()
for i in 1 2 3 4 5; do
    "$BUILD/hold" "$portfile" 3000 &
    hold_pids+=($!)
done
sleep 0.3
echo "服务器线程数（5 个连接）: $(ps -o nlwp= -p "$srv" | tr -d ' ')"

start=$(date +%s%N)
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k" > "$BUILD/get.log"
end=$(date +%s%N)
echo "B 的 GET 用时: $(( (end - start) / 1000000 )) ms"
cat "$BUILD/get.log"

for p in "${hold_pids[@]}"; do wait "$p"; done
sleep 0.2
kill "$srv"
wait "$srv" || true
cat "$BUILD/server.log"
