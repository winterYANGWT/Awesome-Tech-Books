#!/usr/bin/env bash
# mode: run
# timeout: 30
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

# A 建连后不发数据，迭代版卡在 recv(A) 上。
"$BUILD/hold" "$portfile" 2000 &
hold=$!
sleep 0.2

start=$(date +%s%N)
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k" > "$BUILD/get.log"
end=$(date +%s%N)
echo "B 的 GET 用时: $(( (end - start) / 1000000 )) ms"
cat "$BUILD/get.log"

wait "$hold"
sleep 0.2
kill "$srv"
wait "$srv" || true
cat "$BUILD/server.log"
