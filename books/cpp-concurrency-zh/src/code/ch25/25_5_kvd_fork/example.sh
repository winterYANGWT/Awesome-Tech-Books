#!/usr/bin/env bash
# mode: run
# timeout: 40
# output: varies
# expect: (?s)conn A: SET k 1.*OK.*conn B: GET k.*NOT_FOUND.*server: connections=3 reaped=3.*residual zombie children: 0
set -euo pipefail

"$CXX" $CXXFLAGS -I../../kvd server.cpp -o "$BUILD/server"
"$CXX" $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"
"$CXX" $CXXFLAGS -I../../kvd hold_client.cpp -o "$BUILD/hold"

portfile="$BUILD/kvd.port"
rm -f "$portfile"
"$BUILD/server" --port-file "$portfile" > "$BUILD/server.log" 2>&1 &
srv=$!
cleanup() {
    kill "$srv" 2>/dev/null || true
    wait "$srv" 2>/dev/null || true
    rm -f "$portfile"
}
trap cleanup EXIT

for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

echo "conn A: SET k 1"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "SET k 1"
echo "conn B: GET k"
timeout 10 "$BUILD/kvbench" --port-file "$portfile" --once "GET k"

"$BUILD/hold" "$portfile" 3000 &
hold=$!
sleep 0.3

echo "server and children:"
ps -o pid,ppid,stat,comm -p "$srv" --ppid "$srv" || true

child_pids=$(ps -o pid= --ppid "$srv" | tr -d ' ')
kill "$srv"
wait "$srv" || true
wait "$hold" || true
cat "$BUILD/server.log"

residual=0
for p in $child_pids; do
    if [ -r "/proc/$p/stat" ]; then
        state=$(awk '{print $3}' "/proc/$p/stat" 2>/dev/null || true)
        [ "$state" = "Z" ] && residual=$((residual + 1))
    fi
done
echo "residual zombie children: $residual"
