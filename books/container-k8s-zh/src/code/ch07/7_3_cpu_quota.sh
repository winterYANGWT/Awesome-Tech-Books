# 配额：--cpus 0.5 让一个死循环最多用半个 CPU。
docker run -d --name shorty-cpu --network none --cpus 0.5 busybox:1.37 sh -c 'while :; do :; done' > /dev/null
pid=$(docker inspect -f '{{.State.Pid}}' shorty-cpu)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
for _ in $(seq 1 50); do
    n=$(awk '$1 == "nr_throttled" {print $2}' "/sys/fs/cgroup$cg/cpu.stat")
    [ "${n:-0}" -gt 0 ] && break
    sleep 0.2
done
printf 'cpu.max: %s\n' "$(cat "/sys/fs/cgroup$cg/cpu.max")"
awk '$1 ~ /^(nr_periods|nr_throttled|throttled_usec)$/' "/sys/fs/cgroup$cg/cpu.stat"
docker stats --no-stream shorty-cpu

# 权重：--cpu-shares 512 不改 cpu.max，也不产生 throttle；空闲时它照样跑满一个核。
docker run -d --name shorty-shares --network none --cpu-shares 512 busybox:1.37 sh -c 'while :; do :; done' > /dev/null
pid=$(docker inspect -f '{{.State.Pid}}' shorty-shares)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
for _ in $(seq 1 50); do
    n=$(awk '$1 == "usage_usec" {print $2}' "/sys/fs/cgroup$cg/cpu.stat")
    [ "${n:-0}" -gt 1000000 ] && break
    sleep 0.2
done
printf 'cpu.max: %s\n' "$(cat "/sys/fs/cgroup$cg/cpu.max")"
awk '$1 ~ /^(nr_periods|nr_throttled|throttled_usec)$/' "/sys/fs/cgroup$cg/cpu.stat"
docker stats --no-stream shorty-shares

docker inspect -f 'shorty-cpu NanoCpus={{.HostConfig.NanoCpus}} CpuShares={{.HostConfig.CpuShares}}' shorty-cpu
docker inspect -f 'shorty-shares NanoCpus={{.HostConfig.NanoCpus}} CpuShares={{.HostConfig.CpuShares}}' shorty-shares

#> hide
docker rm -f shorty-cpu shorty-shares > /dev/null
