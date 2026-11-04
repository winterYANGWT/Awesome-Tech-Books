#!/usr/bin/env bash
# mode: run
# timeout: 90
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/hsha"
"$CXX" $CXXFLAGS -I../../kvd ../../ch32/32_5_kvd_epoll/server.cpp -o "$BUILD/epoll"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

bench() {
    local name=$1
    shift
    local portfile="$BUILD/$name.port"
    local logfile="$BUILD/$name.log"
    rm -f "$portfile"
    "$@" --port-file "$portfile" > "$logfile" 2>&1 &
    local srv=$!
    for _ in $(seq 1 300); do
        [ -f "$portfile" ] && break
        sleep 0.01
    done
    [ -f "$portfile" ]
    echo "$name:"
    timeout 60 "$BUILD/kvbench" --port-file "$portfile" -c 100 -n 200 \
        --pipeline 4 --set-ratio 0.1
    kill "$srv" 2>/dev/null || true
    wait "$srv" 2>/dev/null || true
    cat "$logfile"
}

bench "epoll" "$BUILD/epoll" --mode lt
bench "hsha" "$BUILD/hsha" --workers 8 --queue-cap 1024
