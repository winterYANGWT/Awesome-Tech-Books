# 未设限制的 api：从 pid 找到 cgroup，再读控制器文件。
pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
echo "api pid=$pid"
cat "/proc/$pid/cgroup"
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
echo "cgroup 目录=/sys/fs/cgroup$cg"
for f in memory.max cpu.max cpu.weight pids.max cpuset.cpus.effective; do
    printf '%-22s %s\n' "$f" "$(cat "/sys/fs/cgroup$cg/$f")"
done

# 带全部限制的探针容器；busybox 只用来占位。
docker run -d --name shorty-cg --network none \
    --memory 64m --memory-swap 64m --cpus 0.5 \
    --cpu-shares 512 --cpuset-cpus 0 --pids-limit 64 \
    busybox:1.37 sleep 300 > /dev/null
#> expect: 67108864 67108864 500000000 512 0 64
docker inspect -f '{{.HostConfig.Memory}} {{.HostConfig.MemorySwap}} {{.HostConfig.NanoCpus}} {{.HostConfig.CpuShares}} {{.HostConfig.CpusetCpus}} {{.HostConfig.PidsLimit}}' shorty-cg

# 同一个容器，在内核侧看到的 cgroup v2 文件。
pid=$(docker inspect -f '{{.State.Pid}}' shorty-cg)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
for f in memory.max memory.swap.max cpu.max cpu.weight cpuset.cpus pids.max; do
    printf '%-18s %s\n' "$f" "$(cat "/sys/fs/cgroup$cg/$f")"
done

# --memory-swap 不设时的默认值：MemorySwap 是内存的两倍。
docker run -d --name shorty-swap --network none --memory 64m \
    busybox:1.37 sleep 300 > /dev/null
pid=$(docker inspect -f '{{.State.Pid}}' shorty-swap)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
printf 'MemorySwap=%s memory.swap.max=%s\n' \
    "$(docker inspect -f '{{.HostConfig.MemorySwap}}' shorty-swap)" \
    "$(cat "/sys/fs/cgroup$cg/memory.swap.max")"

# 把 api 本身设成本章终点的限制。
docker update --memory 128m --memory-swap 128m --cpus 0.5 shorty-api
pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
docker inspect -f 'api HostConfig: Memory={{.HostConfig.Memory}} MemorySwap={{.HostConfig.MemorySwap}} NanoCpus={{.HostConfig.NanoCpus}}' shorty-api
for f in memory.max memory.swap.max cpu.max; do
    printf '%-18s %s\n' "$f" "$(cat "/sys/fs/cgroup$cg/$f")"
done

#> hide
docker rm -f shorty-cg shorty-swap > /dev/null
