#!/usr/bin/env bash
# mode: run
# timeout: 300
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd ../../ch32/32_5_kvd_epoll/server.cpp -o "$BUILD/epoll"
"$CXX" $CXXFLAGS -I../../kvd ../35_3_kvd_hsha/server.cpp -o "$BUILD/hsha"
"$CXX" $CXXFLAGS -I../../kvd ../35_4_kvd_reuseport/server.cpp -o "$BUILD/reuseport"
"$CXX" $CXXFLAGS -I../../kvd ../../ch27/27_6_kvd_shm/server.cpp -o "$BUILD/shm"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

run_one() {
    local name=$1
    local conns=$2
    local n=$3
    local portfile="$BUILD/$name.$conns.port"
    local logfile="$BUILD/$name.$conns.log"
    rm -f "$portfile"
    case "$name" in
        epoll)     "$BUILD/epoll" --port-file "$portfile" --mode lt >"$logfile" 2>&1 & ;;
        hsha)      "$BUILD/hsha" --port-file "$portfile" --workers 8 >"$logfile" 2>&1 & ;;
        reuseport) "$BUILD/reuseport" --port-file "$portfile" --workers 4 >"$logfile" 2>&1 & ;;
        shm)       "$BUILD/shm" --port-file "$portfile" --workers 4 >"$logfile" 2>&1 & ;;
    esac
    local srv=$!
    for _ in $(seq 1 300); do
        [ -f "$portfile" ] && break
        sleep 0.01
    done
    if [ ! -f "$portfile" ]; then
        echo "server $name failed to start" >&2
        cat "$logfile" >&2
        kill "$srv" 2>/dev/null || true
        wait "$srv" 2>/dev/null || true
        return 1
    fi
    local line
    line=$(timeout 60 "$BUILD/kvbench" --port-file "$portfile" -c "$conns" -n "$n" \
        --set-ratio 0.1 --csv | tail -n 1)
    local thr p99
    thr=$(printf '%s\n' "$line" | cut -d, -f4)
    p99=$(printf '%s\n' "$line" | cut -d, -f6)
    printf '%-10s %5s %12s %8s\n' "$name" "$conns" "$thr" "$p99"
    kill "$srv" 2>/dev/null || true
    wait "$srv" 2>/dev/null || true
}

echo "structure,conns,throughput,p99_us"
for conns in 1 10 100; do
    n=$((20000 / conns))
    run_one epoll "$conns" "$n"
    run_one hsha "$conns" "$n"
    run_one reuseport "$conns" "$n"
    run_one shm "$conns" "$n"
done
