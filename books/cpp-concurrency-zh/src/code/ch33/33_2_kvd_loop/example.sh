#!/usr/bin/env bash
# mode: run
# timeout: 60
# output: varies
set -euo pipefail

mkdir -p "$BUILD"
"$CXX" $CXXFLAGS -I../../kvd event_loop.cpp server.cpp -o "$BUILD/kvd_loop"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"
"$CXX" $CXXFLAGS -I../../kvd hold_client.cpp -o "$BUILD/hold_client"

portfile="$BUILD/kvd.port"
logfile="$BUILD/server.log"
rm -f "$portfile"
"$BUILD/kvd_loop" --port-file "$portfile" --idle-timeout 2 > "$logfile" 2>&1 &
srv=$!
for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 30 "$BUILD/kvbench" --port-file "$portfile" -c 8 -n 2000 --pipeline 4
timeout 10 "$BUILD/hold_client" --port-file "$portfile" --seconds 3
sleep 0.2
kill -TERM "$srv"
wait "$srv"
cat "$logfile"
