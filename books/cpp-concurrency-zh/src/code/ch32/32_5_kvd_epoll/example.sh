#!/usr/bin/env bash
# mode: run
# timeout: 60
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

for mode in lt et; do
    portfile="$BUILD/kvd.$mode.port"
    logfile="$BUILD/server.$mode.log"
    rm -f "$portfile"
    "$BUILD/server" --port-file "$portfile" --mode "$mode" > "$logfile" 2>&1 &
    srv=$!
    for _ in $(seq 1 200); do
        [ -f "$portfile" ] && break
        sleep 0.01
    done
    [ -f "$portfile" ]

    echo "mode=$mode"
    timeout 30 "$BUILD/kvbench" --port-file "$portfile" -c 8 -n 2000 --pipeline 4
    kill "$srv"
    wait "$srv" || true
    cat "$logfile"
done
