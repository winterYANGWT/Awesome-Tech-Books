#!/usr/bin/env bash
# mode: run
# timeout: 90
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

portfile="$BUILD/kvd.port"
logfile="$BUILD/server.log"
holdfile="$BUILD/hold.pid"
rm -f "$portfile" "$holdfile"

"$BUILD/server" --port-file "$portfile" --workers 4 --crash-hold-file "$holdfile" \
    > "$logfile" 2>&1 &
srv=$!
cleanup() {
    kill "$srv" 2>/dev/null || true
    wait "$srv" 2>/dev/null || true
    rm -f "$portfile" "$holdfile"
}
trap cleanup EXIT

for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

echo "客户端 A: SET k 1"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "SET k 1"
echo "客户端 B: GET k"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k"

echo "混合读写，4 个连接:"
timeout 20 "$BUILD/kvbench" --port-file "$portfile" -c 4 -n 200 --set-ratio 0.1 --quiet

echo "让一个 worker 持锁停住，然后 kill -9:"
timeout 30 "$BUILD/kvbench" --port-file "$portfile" --once "SET crash 1" \
    > "$BUILD/crash-client.log" 2>&1 &
crash_client=$!

for _ in $(seq 1 500); do
    [ -s "$holdfile" ] && break
    sleep 0.01
done
if [ ! -s "$holdfile" ]; then
    echo "worker 没有进入持锁状态"
    exit 1
fi
victim=$(cat "$holdfile")
echo "kill -9 worker $victim"
kill -9 "$victim"

# 客户端所在的连接随 worker 一起断开，退出码非 0 是预期的。
if ! wait "$crash_client"; then :; fi

for _ in $(seq 1 500); do
    grep -q "新池已就绪" "$logfile" && break
    sleep 0.01
done
grep -q "新池已就绪" "$logfile"

echo "恢复后 GET k（旧表已丢弃）:"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k"
echo "恢复后 SET k 2 再 GET:"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "SET k 2"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k"
echo "恢复后再跑一轮混合读写:"
timeout 20 "$BUILD/kvbench" --port-file "$portfile" -c 4 -n 200 --set-ratio 0.1 --quiet

kill "$srv"
wait "$srv" || true
trap - EXIT
cat "$logfile"
