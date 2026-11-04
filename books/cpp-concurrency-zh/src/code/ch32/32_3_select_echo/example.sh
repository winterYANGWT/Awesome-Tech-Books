#!/usr/bin/env bash
# mode: run
# timeout: 30
set -euo pipefail

"$CC" $CFLAGS server.c -o "$BUILD/server"
"$CC" $CFLAGS client.c -o "$BUILD/client"

portfile="$BUILD/server.port"
logfile="$BUILD/server.log"
rm -f "$portfile"
"$BUILD/server" "$portfile" > "$logfile" 2>&1 &
srv=$!
trap 'kill "$srv" 2>/dev/null || true; wait "$srv" 2>/dev/null || true; rm -f "$portfile"' EXIT

for _ in $(seq 1 200); do
    [ -f "$portfile" ] && break
    sleep 0.01
done
[ -f "$portfile" ]

timeout 10 "$BUILD/client" "$portfile"
timeout 10 "$BUILD/client" "$portfile"

wait "$srv"
cat "$logfile"
