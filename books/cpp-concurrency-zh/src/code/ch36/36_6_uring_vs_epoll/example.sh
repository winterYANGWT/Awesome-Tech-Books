set -euo pipefail
# output: varies
# 同一套 kvd 请求-响应协议、同一组 kvbench 参数，epoll 版和 io_uring 版各跑一轮。
# 标准输出是整个 CSV（给 chart 读）；构建日志和中间结果都写进 $BUILD。

$CXX $CXXFLAGS -I../../kvd epoll_server.cpp -o "$BUILD/epoll_server"
$CXX $CXXFLAGS -I../../kvd uring_server.cpp -o "$BUILD/uring_server" -luring
$CXX $CXXFLAGS -I../../kvd ../../kvd/kvbench.cpp -o "$BUILD/kvbench"

server_pid=
cleanup() {
    if [ -n "$server_pid" ]; then
        kill -TERM "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT

: >"$BUILD/rows"
conns_list="4 16 64"

for conns in $conns_list; do
    for impl in epoll uring; do
        port_file="$BUILD/$impl.$conns.port"
        log="$BUILD/$impl.$conns.log"
        rm -f "$port_file" "$log"
        "$BUILD/${impl}_server" --port-file "$port_file" >"$log" 2>&1 &
        server_pid=$!
        for _ in $(seq 1 200); do
            if [ -s "$port_file" ]; then break; fi
            sleep 0.01
        done
        if [ ! -s "$port_file" ]; then
            echo "server $impl/$conns did not start" >&2
            cat "$log" >&2
            kill -TERM "$server_pid" 2>/dev/null || true
            wait "$server_pid" 2>/dev/null || true
            exit 1
        fi

        line=$(timeout 30 "$BUILD/kvbench" --port-file "$port_file" \
            -c "$conns" -n 400 --pipeline 4 --csv | tail -n 1)
        kill -TERM "$server_pid"
        wait "$server_pid"
        server_pid=
        syscalls=$(sed -n 's/^syscalls=//p' "$log" | tail -n 1)
        throughput=$(echo "$line" | cut -d, -f4)
        throughput=$(awk -v x="$throughput" 'BEGIN { printf "%.2f", x / 1000000 }')
        p99=$(echo "$line" | cut -d, -f6)
        echo "$conns,$impl,$throughput,$p99,$syscalls" >>"$BUILD/rows"
    done
done

echo "conns,epoll,uring"
for conns in $conns_list; do
    awk -F, -v c="$conns" '
        $1 == c && $2 == "epoll" { e = $3 }
        $1 == c && $2 == "uring" { u = $3 }
        END { print c "," e "," u }
    ' "$BUILD/rows"
done

{
    echo "conns,epoll,uring"
    for conns in $conns_list; do
        awk -F, -v c="$conns" '
            $1 == c && $2 == "epoll" { e = $4 }
            $1 == c && $2 == "uring" { u = $4 }
            END { print c "," e "," u }
        ' "$BUILD/rows"
    done
} >p99.out

{
    echo "conns,epoll,uring"
    for conns in $conns_list; do
        awk -F, -v c="$conns" '
            $1 == c && $2 == "epoll" { e = $5 }
            $1 == c && $2 == "uring" { u = $5 }
            END { print c "," e "," u }
        ' "$BUILD/rows"
    done
} >syscalls.out
