# 普通容器的 capability 集合。
docker run --rm lab/busybox:1.37 grep CapEff /proc/self/status

# 在 lab 的 network namespace 里起一个 shorty，监听 127.0.0.1:18082。
SHORTY_LISTEN=127.0.0.1:18082 /tmp/ch01-shorty serve >/tmp/ch01-shorty-net.log 2>&1 &
srv=$!
for _ in $(seq 1 50); do
    curl -fsS http://127.0.0.1:18082/healthz >/dev/null 2>&1 && break
done

docker run -d --name host-risk --pid=host --network=host lab/busybox:1.37 sleep 600 >/dev/null

echo '--- 容器里看到的进程（宿主 PID namespace）---'
#> expect: docker-init
docker exec host-risk ps -eo pid,ppid,comm | head -12
echo '--- 容器里的 PID 1 ---'
docker exec host-risk cat /proc/1/cmdline | tr '\0' ' '
echo
echo '--- 容器里看到 lab 的监听端口 ---'
#> expect: 127.0.0.1:18082
docker exec host-risk netstat -ltn | grep 18082
echo '--- 容器直接访问 127.0.0.1:18082 ---'
#> expect: ok
docker exec host-risk wget -qO- http://127.0.0.1:18082/healthz
echo

#> expect: PidMode=host NetworkMode=host
docker inspect -f 'PidMode={{.HostConfig.PidMode}} NetworkMode={{.HostConfig.NetworkMode}}' host-risk
echo '--- host-pid 容器的 CapEff ---'
docker exec host-risk grep CapEff /proc/self/status
echo '--- --privileged 容器的 CapEff ---'
#> expect: 000001ffffffffff
docker run --rm --privileged lab/busybox:1.37 grep CapEff /proc/self/status

docker rm -f host-risk >/dev/null
kill "$srv"
wait "$srv" 2>/dev/null || true
