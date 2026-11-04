#!/usr/bin/env bash
# mode: run
# timeout: 30
# expect: second connection timed out
# expect: accepted=1
set -euo pipefail

"$CC" $CFLAGS server.c -o "$BUILD/server"
"$CC" $CFLAGS client.c -o "$BUILD/client"

portfile="$BUILD/port"
readyfile="$BUILD/ready"
rm -f "$portfile" "$readyfile"

"$BUILD/server" "$portfile" "$readyfile" > "$BUILD/server.log" 2>&1 &
srv=$!
trap 'kill "$srv" 2>/dev/null || true; wait "$srv" 2>/dev/null || true' EXIT

for _ in $(seq 1 500); do
    [ -s "$portfile" ] && break
    sleep 0.01
done
[ -s "$portfile" ]

timeout 15 "$BUILD/client" "$portfile" "$readyfile"
wait "$srv"
cat "$BUILD/server.log"
