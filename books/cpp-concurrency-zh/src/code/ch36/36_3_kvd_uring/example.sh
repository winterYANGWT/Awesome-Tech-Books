set -euo pipefail
# 构建产物全部放进 $BUILD，标准输出只留压测结果和服务器最后一行。

$CXX $CXXFLAGS -I../../kvd kvd_uring.cpp -o "$BUILD/kvd_uring" -luring
$CXX $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

rm -f "$BUILD/port"
"$BUILD/kvd_uring" --port-file "$BUILD/port" >"$BUILD/server.log" 2>&1 &
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
