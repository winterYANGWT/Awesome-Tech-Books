set -euo pipefail
# mode: run
# expect: client: received ping
# timeout: 30

$CC $CFLAGS server.c -o "$BUILD/server"
$CC $CFLAGS client.c -o "$BUILD/client"

portfile="$BUILD/port"
rm -f "$portfile"

timeout 10 "$BUILD/server" parent "$portfile" &
srv=$!
trap 'kill "$srv" 2>/dev/null || true' EXIT

for _ in $(seq 1 100); do
    if [ -s "$portfile" ]; then
        break
    fi
    sleep 0.05
done
if [ ! -s "$portfile" ]; then
    echo "server did not write the port file" >&2
    exit 1
fi

port=$(cat "$portfile")
timeout 10 "$BUILD/client" "$port"
wait "$srv"
