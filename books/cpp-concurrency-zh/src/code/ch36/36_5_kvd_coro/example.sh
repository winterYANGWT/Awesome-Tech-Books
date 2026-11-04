set -euo pipefail
# kvd_coro + kvbench；SIGTERM 后服务器取消所有连接协程，等 io_context 排空。
# Asio 是 header-only，链接只要 -pthread。

$CXX $CXXFLAGS -I../../kvd kvd_coro.cpp -o "$BUILD/kvd_coro"
$CXX $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

rm -f "$BUILD/port"
"$BUILD/kvd_coro" --port-file "$BUILD/port" >"$BUILD/server.log" 2>&1 &
server_pid=$!
trap 'kill -TERM "$server_pid" 2>/dev/null || true; wait "$server_pid" 2>/dev/null || true' EXIT

for _ in $(seq 1 200); do
    if [ -s "$BUILD/port" ]; then break; fi
    sleep 0.01
done
if [ ! -s "$BUILD/port" ]; then
    echo "server did not write a port" >&2
    cat "$BUILD/server.log" >&2
    exit 1
fi

timeout 20 "$BUILD/kvbench" --port-file "$BUILD/port" -c 16 -n 500 --pipeline 4 --csv

kill -TERM "$server_pid"
wait "$server_pid"
trap - EXIT
cat "$BUILD/server.log"
