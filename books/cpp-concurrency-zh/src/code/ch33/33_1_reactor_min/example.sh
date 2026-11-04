#!/usr/bin/env bash
# mode: run
# timeout: 30
# output: varies
set -euo pipefail

mkdir -p "$BUILD"
"$CXX" $CXXFLAGS -I../../kvd event_loop.cpp timer_queue.cpp connection.cpp acceptor.cpp main.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd client.cpp -o "$BUILD/client"

portfile="$BUILD/reactor.port"
logfile="$BUILD/server.log"
rm -f "$portfile"
"$BUILD/server" --port-file "$portfile" > "$logfile" 2>&1 &
srv=$!
for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 10 "$BUILD/client" --port-file "$portfile" --mode active --seconds 2 > "$BUILD/active.log" 2>&1 &
active=$!
timeout 10 "$BUILD/client" --port-file "$portfile" --mode idle --seconds 4 > "$BUILD/idle.log" 2>&1 &
idle=$!

wait "$active"
wait "$idle"
sleep 0.3
kill "$srv" 2>/dev/null || true
wait "$srv" 2>/dev/null || true

cat "$BUILD/active.log"
cat "$BUILD/idle.log"
cat "$logfile"
