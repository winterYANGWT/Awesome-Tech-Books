#!/usr/bin/env bash
# mode: run
# timeout: 60
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server" -levent
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

portfile="$BUILD/kvd-libevent.port"
rm -f "$portfile"
"$BUILD/server" --port-file "$portfile" > "$BUILD/server.log" 2>&1 &
srv=$!
trap 'kill "$srv" 2>/dev/null || true; wait "$srv" 2>/dev/null || true; rm -f "$portfile"' EXIT

for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 30 "$BUILD/kvbench" --port-file "$portfile" -c 8 -n 2000 --pipeline 4
kill -TERM "$srv"
wait "$srv" || true
cat "$BUILD/server.log"
