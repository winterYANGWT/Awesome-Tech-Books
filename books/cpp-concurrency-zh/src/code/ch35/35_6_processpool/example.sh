#!/usr/bin/env bash
# mode: run
# timeout: 60
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd client.cpp -o "$BUILD/client"

portfile="$BUILD/kvd.port"
logfile="$BUILD/server.log"
rm -f "$portfile"

"$BUILD/server" --port-file "$portfile" --children 4 > "$logfile" 2>&1 &
srv=$!
cleanup() {
    kill "$srv" 2>/dev/null || true
    wait "$srv" 2>/dev/null || true
    rm -f "$portfile"
}
trap cleanup EXIT

for _ in $(seq 1 300); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

echo "20 条连接，4 个子进程轮流 accept："
timeout 20 "$BUILD/client" --port-file "$portfile" --connections 20

sleep 0.2
kill "$srv"
wait "$srv" || true
trap - EXIT
cat "$logfile"
