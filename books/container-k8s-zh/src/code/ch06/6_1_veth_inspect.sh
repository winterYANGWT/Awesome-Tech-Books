# veth 的两端：容器里的 eth0 和宿主上的 vethXXXX。容器是默认 bridge 上的一个端点。
docker run -d --name ch06-veth busybox:1.37 sleep 600 >/dev/null

# 默认 bridge 的网段和网关，以 daemon 实际配置为准。
docker network inspect bridge --format '{{range .IPAM.Config}}subnet={{.Subnet}} gateway={{.Gateway}}{{end}}'

# 容器进程持有的 netns。Docker 不把它注册到 /run/netns，所以 ip netns 看不到。
pid=$(docker inspect -f '{{.State.Pid}}' ch06-veth)
printf 'container pid=%s netns=%s\n' "$pid" "$(readlink "/proc/$pid/ns/net")"
ip netns list 2>/dev/null || true

# 容器里的接口和默认路由。
docker exec ch06-veth ip -o addr show eth0
docker exec ch06-veth ip route

# eth0 的对端 ifindex 就是宿主 veth 的 ifindex；用它从宿主这一侧找到配对接口。
host_ifindex=$(docker exec ch06-veth cat /sys/class/net/eth0/iflink)
host_if=$(ip -o link | awk -F': ' -v n="$host_ifindex" '$1 + 0 == n {sub(/@.*/, "", $2); print $2}')
printf 'eth0 iflink=%s host veth=%s\n' "$host_ifindex" "$host_if"
bridge link show dev "$host_if"
#> expect: docker0
ip route | grep docker0

docker rm -f ch06-veth >/dev/null
