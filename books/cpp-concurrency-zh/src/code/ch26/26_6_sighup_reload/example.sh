set -euo pipefail
# expect: 第一次：VALUE old
# expect: SIGHUP 后：VALUE new
$CC $CFLAGS server.c -o "$BUILD/server"
$CC $CFLAGS client.c -o "$BUILD/client"
tmpdir=$(mktemp -d)
srv=
cleanup() {
  if [ -n "${srv:-}" ]; then kill -TERM "$srv" 2>/dev/null || true; wait "$srv" 2>/dev/null || true; fi
  rm -rf "$tmpdir"
}
trap cleanup EXIT
printf 'old\n' > "$tmpdir/mode"
"$BUILD/server" "$tmpdir/port" "$tmpdir/mode" &
srv=$!
for _ in $(seq 1 100); do
  [ -s "$tmpdir/port" ] && break
  sleep 0.02
done
port=$(cat "$tmpdir/port")
printf '第一次：'
"$BUILD/client" "$port" 'GET mode'
printf 'new\n' > "$tmpdir/mode"
kill -HUP "$srv"
sleep 0.2
printf 'SIGHUP 后：'
"$BUILD/client" "$port" 'GET mode'
kill -TERM "$srv"
wait "$srv"
srv=
