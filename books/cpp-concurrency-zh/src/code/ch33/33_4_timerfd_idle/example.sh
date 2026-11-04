#!/usr/bin/env bash
# mode: run
# timeout: 40
# output: varies
set -euo pipefail

mkdir -p "$BUILD"
"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd client.cpp -o "$BUILD/client"

portfile="$BUILD/idle.port"
logfile="$BUILD/server.log"
rm -f "$portfile"
"$BUILD/server" --port-file "$portfile" > "$logfile" 2>&1 &
srv=$!
for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 15 "$BUILD/client" --port-file "$portfile" --mode idle --seconds 5 > "$BUILD/idle.log" 2>&1 &
idle=$!
timeout 15 "$BUILD/client" --port-file "$portfile" --mode active --seconds 6 > "$BUILD/active.log" 2>&1 &
active=$!

wait "$idle"
wait "$active"
sleep 0.2
kill "$srv" 2>/dev/null || true
wait "$srv" 2>/dev/null || true

cat "$BUILD/idle.log"
cat "$BUILD/active.log"
cat "$logfile"
