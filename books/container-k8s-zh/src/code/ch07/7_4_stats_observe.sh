# 起四个并发客户端压 api；它们只发请求，不写数据。
docker run -d --name shorty-load --network shorty-net busybox:1.37 sh -c '
    i=0
    while [ $i -lt 2000 ]; do
        wget -q -O /dev/null http://shorty-api:8080/healthz
        i=$((i + 1))
    done
' > /dev/null

# docker stats 是 Docker 自己的采样；MEM USAGE / LIMIT 的右边就是 128m。
#> expect: 128MiB
docker stats --no-stream shorty-api shorty-load

pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
printf 'memory.current=%s bytes\n' "$(cat "/sys/fs/cgroup$cg/memory.current")"
printf 'memory.peak=%s bytes\n' "$(cat "/sys/fs/cgroup$cg/memory.peak")"
grep -E '^(anon|file|inactive_file|active_file) ' "/sys/fs/cgroup$cg/memory.stat"
awk '$1 ~ /^(usage_usec|nr_periods|nr_throttled|throttled_usec)$/' "/sys/fs/cgroup$cg/cpu.stat"
cat "/sys/fs/cgroup$cg/cgroup.events"

docker inspect -f 'State: {{.State.Status}} OOMKilled={{.State.OOMKilled}}' shorty-api
docker inspect -f 'HostConfig: Memory={{.HostConfig.Memory}} MemorySwap={{.HostConfig.MemorySwap}} NanoCpus={{.HostConfig.NanoCpus}}' shorty-api

#> hide
docker rm -f shorty-load > /dev/null

# dockerd 的 metrics-addr 只在启动时读；lab 里的 dockerd 是 PID 1 的子进程，
# 不能直接重启。这里用同一个二进制在另一个 socket 上起一个临时 daemon 验证。
mkdir -p /tmp/ch07-metrics
dockerd --host=unix:///tmp/ch07-metrics/docker.sock \
    --data-root=/tmp/ch07-metrics/data --exec-root=/tmp/ch07-metrics/exec \
    --pidfile=/tmp/ch07-metrics/dockerd.pid \
    --metrics-addr=127.0.0.1:9323 \
    --iptables=false --ip6tables=false --bridge=none --storage-driver=vfs \
    > /tmp/ch07-metrics/dockerd.log 2>&1 &
dpid=$!
for _ in $(seq 1 60); do
    [ -S /tmp/ch07-metrics/docker.sock ] && break
    sleep 0.2
done
DOCKER_HOST=unix:///tmp/ch07-metrics/docker.sock docker info --format '临时 daemon {{.ServerVersion}}'
curl -sS http://127.0.0.1:9323/metrics \
    | grep -E '^engine_daemon_(engine_info|container_states_containers\{state="running"\})' \
    | head -2
kill "$dpid" 2>/dev/null || true
for _ in $(seq 1 50); do
    kill -0 "$dpid" 2>/dev/null || break
    sleep 0.1
done
kill -9 "$dpid" 2>/dev/null || true
rm -rf /tmp/ch07-metrics || true
