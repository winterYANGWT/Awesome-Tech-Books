#!/usr/bin/env bash
# mode: run
# timeout: 30
# output: varies
set -euo pipefail

mkdir -p "$BUILD"
"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd client.cpp -o "$BUILD/client"

portfile="$BUILD/graceful.port"
logfile="$BUILD/server.log"
rm -f "$portfile"
readyfile="$BUILD/request.ready"
rm -f "$readyfile"
"$BUILD/server" --port-file "$portfile" --ready-file "$readyfile" > "$logfile" 2>&1 &
srv=$!
for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 10 "$BUILD/client" --port-file "$portfile" > "$BUILD/client.log" 2>&1 &
cli=$!
for _ in $(seq 1 200); do
    [ -f "$readyfile" ] && break
    sleep 0.01
done
[ -f "$readyfile" ]
kill -TERM "$srv"
wait "$cli"
wait "$srv"
cat "$BUILD/client.log"
cat "$logfile"
