#!/usr/bin/env bash
# mode: run
# timeout: 90
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

portfile="$BUILD/kvd.port"
logfile="$BUILD/server.log"
rm -f "$portfile"

"$BUILD/server" --port-file "$portfile" --workers 4 > "$logfile" 2>&1 &
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

timeout 60 "$BUILD/kvbench" --port-file "$portfile" -c 200 -n 20 --set-ratio 0.1
sleep 0.1
kill "$srv"
wait "$srv" || true
trap - EXIT
cat "$logfile"
