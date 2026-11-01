# 在一个容器里读自己的 namespace，再从宿主读同一个进程的 namespace，对比 inode。
docker run -d --name ns-demo lab/busybox:1.37 sleep 600 >/dev/null
pid=$(docker inspect -f '{{.State.Pid}}' ns-demo)
echo "容器里的 PID 1 在宿主上是 PID $pid"
printf '%-6s %-12s %-12s %-12s %s\n' ns container host-same-pid host-init result
#> expect: pid\s+\d+\s+\d+\s+\d+\s+separate from host init
#> expect: user\s+\d+\s+\d+\s+\d+\s+shared with host init
for ns in pid mnt uts ipc net user; do
    container=$(docker exec ns-demo stat -Lc %i "/proc/1/ns/$ns")
    host=$(stat -Lc %i "/proc/$pid/ns/$ns")
    init=$(stat -Lc %i /proc/1/ns/$ns)
    if [ "$container" = "$init" ]; then
        result="shared with host init"
    else
        result="separate from host init"
    fi
    printf '%-6s %-12s %-12s %-12s %s\n' "$ns" "$container" "$host" "$init" "$result"
done

echo '--- 容器里看 /proc/1/status ---'
#> expect: NSpid:\s+1
docker exec ns-demo cat /proc/1/status | grep -E '^(NSpid|CapEff)'
echo '--- 宿主看同一个进程的 /proc/'"$pid"'/status ---'
#> expect: NSpid:\s+\d+\s+1
cat "/proc/$pid/status" | grep -E '^(NSpid|CapEff)'

echo '--- 容器里的 uname -a ---'
docker exec ns-demo uname -a
echo '--- 宿主的 uname -a ---'
uname -a

docker rm -f ns-demo >/dev/null
