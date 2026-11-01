# cgroup v2 的控制器要先出现在父 cgroup 的 cgroup.controllers 里，
# 再由 cgroup.subtree_control 启用给子 cgroup；这里根 cgroup 已经启用了 memory/cpu/pids。
cg=/sys/fs/cgroup/ch01demo
if [ -d "$cg" ]; then
    echo 1 > "$cg/cgroup.kill" 2>/dev/null || true
    rmdir "$cg" 2>/dev/null || true
fi
mkdir "$cg"
echo "cgroup.controllers: $(cat /sys/fs/cgroup/cgroup.controllers)"
echo "cgroup.subtree_control: $(cat /sys/fs/cgroup/cgroup.subtree_control)"
echo "根 cgroup 里的进程数: $(wc -l < /sys/fs/cgroup/cgroup.procs)"

echo 64M > "$cg/memory.max"
echo 32 > "$cg/pids.max"
echo "50000 100000" > "$cg/cpu.max"
echo "memory.max=$(cat "$cg/memory.max")"
echo "cpu.max=$(cat "$cg/cpu.max")"
echo "pids.max=$(cat "$cg/pids.max")"

# 用一个新的 bash 把自己的 PID 写进 cgroup.procs，再 exec shorty；
# shorty 就落在这个叶子 cgroup 里，宿主上看到的 PID 不变。
bash ./run_shorty_in_cgroup.bash >/tmp/ch01-shorty-cg.log 2>&1 &
cgpid=$!
for _ in $(seq 1 50); do
    curl -fsS http://127.0.0.1:18080/healthz >/dev/null 2>&1 && break
done
#> expect: cgroup: 0::/ch01demo
echo "shorty 的宿主 PID: $cgpid，cgroup: $(cat "/proc/$cgpid/cgroup")"

#> expect: request \d+ -> 000
# 每个请求分配并保留 8 MiB；进程被杀后 curl 拿不到 HTTP 状态码。
for i in $(seq 1 30); do
    code=$(curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1:18080/healthz || true)
    echo "request $i -> ${code:-failed} memory.current=$(cat "$cg/memory.current")"
    if ! kill -0 "$cgpid" 2>/dev/null; then
        echo "shorty 在第 $i 个请求后消失"
        break
    fi
done
#> expect: wait exit=137
wait "$cgpid"; echo "wait exit=$?"

echo '--- memory.events ---'
#> expect: oom_kill 1
cat "$cg/memory.events"
echo '--- memory.peak ---'
cat "$cg/memory.peak"
echo "cgroup.procs 剩余进程数: $(wc -l < "$cg/cgroup.procs")"
rmdir "$cg"
if [ -d "$cg" ]; then echo "cgroup 仍然存在"; else echo "cgroup 已删除"; fi
