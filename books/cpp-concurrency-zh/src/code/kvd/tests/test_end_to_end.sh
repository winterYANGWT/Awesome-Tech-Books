#!/usr/bin/env bash
set -euo pipefail

: "${BUILD:?BUILD must be the directory containing ref_server and kvbench}"
SERVER_BIN="$BUILD/ref_server"
BENCH_BIN="$BUILD/kvbench"
WORK="$BUILD/e2e"
PORT_FILE="$WORK/port"

rm -rf "$WORK"
mkdir -p "$WORK"
rm -f "$PORT_FILE"

"$SERVER_BIN" 0 "$PORT_FILE" >"$WORK/server.log" 2>&1 &
SERVER_PID=$!

cleanup() {
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        for _ in {1..100}; do
            kill -0 "$SERVER_PID" 2>/dev/null || break
            sleep 0.01
        done
        kill -KILL "$SERVER_PID" 2>/dev/null || true
    fi
    wait "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT
trap 'exit 1' INT TERM

# 服务器把端口号写进文件；等它出现，最多 10 秒。
if ! timeout 10 bash -c 'until [ -s "$1" ]; do sleep 0.01; done' _ "$PORT_FILE"; then
    cat "$WORK/server.log"
    echo "ref_server did not write $PORT_FILE" >&2
    exit 1
fi
PORT=$(cat "$PORT_FILE")

# 8 个连接、每个 2000 个请求、每个连接 4 个在途请求。
if ! timeout 60 "$BENCH_BIN" --port "$PORT" -c 8 -n 2000 --pipeline 4 \
        >"$WORK/bench.out"; then
    cat "$WORK/bench.out"
    cat "$WORK/server.log"
    exit 1
fi
grep -q 'requests=16000 errors=0 mismatches=0' "$WORK/bench.out"

once() {
    local expected=$1 request=$2 output
    output=$(timeout 10 "$BENCH_BIN" --port "$PORT" --once "$request")
    if [ "$output" != "$expected" ]; then
        echo "once [$request]: want [$expected], got [$output]" >&2
        exit 1
    fi
}

once OK "SET e2e hello"
once "VALUE hello" "GET e2e"
once DELETED "DEL e2e"
once NOT_FOUND "GET e2e"

# 格式错误的请求要打印 ERROR，并以非 0 退出。
if timeout 10 "$BENCH_BIN" --port "$PORT" --once "NOPE x" \
        >"$WORK/error.out" 2>&1; then
    echo "once [NOPE x] should fail" >&2
    exit 1
fi
grep -q '^ERROR ' "$WORK/error.out"

cleanup
trap - EXIT
echo "test_end_to_end: OK"
