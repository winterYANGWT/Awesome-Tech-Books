# 8.2 默认 capability、drop ALL、按需加回
# 默认 CapEff 不是 0：Docker 保留了一组它认为常用且风险较低的 capability。
docker run --rm shorty:dev grep -E 'CapEff|CapBnd' /proc/self/status

# drop ALL 之后进程的 effective capability 变成 0。
#> expect: CapEff:\s+0000000000000000
docker run --rm --cap-drop=ALL shorty:dev grep -E 'CapEff|CapBnd' /proc/self/status
capsh --decode=$(docker run --rm shorty:dev awk '/^CapEff/ {print $2}' /proc/self/status)

# busybox 的 ping 走原始套接字，默认有 CAP_NET_RAW，所以能通。
docker run --rm busybox:1.37 ping -c1 127.0.0.1

# 没有 CAP_NET_RAW 时，socket(AF_INET, SOCK_RAW) 被内核拒绝。
#> fails
#> expect: permission denied
docker run --rm --cap-drop=ALL busybox:1.37 ping -c1 127.0.0.1

# 只把 NET_RAW 加回来，其他仍然全 drop。
docker run --rm --cap-drop=ALL --cap-add=NET_RAW busybox:1.37 ping -c1 127.0.0.1

# --privileged：生效集合变成宿主允许的几乎全部 capability。
docker run --rm --privileged busybox:1.37 grep -E 'CapEff|CapBnd' /proc/self/status

# no-new-privileges 写进进程的 NoNewPrivs 标志，默认是 0。
docker run --rm shorty:dev grep NoNewPrivs /proc/self/status
#> expect: NoNewPrivs:\s+1
docker run --rm --security-opt=no-new-privileges shorty:dev grep NoNewPrivs /proc/self/status
