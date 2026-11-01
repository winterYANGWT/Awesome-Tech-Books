# host 网络：容器直接使用宿主的 netns，没有自己的 IP，也不做端口映射。
docker rm -f shorty-api >/dev/null
docker run -d --name shorty-host --network host -e SHORTY_LISTEN=:8080 shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://localhost:8080/healthz
docker inspect -f 'network_mode={{.HostConfig.NetworkMode}} ports={{json .NetworkSettings.Ports}}' shorty-host

# 容器里看到的网卡就是宿主的网卡；8080 的监听者也直接出现在宿主的 ss 里。
docker exec shorty-host sh -c 'ls /sys/class/net'
ss -lntp | grep ':8080'

# 第二个 host 网络的容器没有端口映射可分配，只能由应用自己 bind，冲突时启动失败。
#> fails
#> expect: address already in use
docker run --rm --name shorty-host-2 --network host -e SHORTY_LISTEN=:8080 shorty:dev /out/shorty serve

docker rm -f shorty-host >/dev/null
docker run -d --name shorty-api --network shorty-net --network-alias api \
    -p 127.0.0.1:8080:8080 \
    -e 'SHORTY_DB_URL=postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable' \
    -e SHORTY_REDIS_ADDR=redis:6379 \
    -e SHORTY_LISTEN=:8080 \
    shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://127.0.0.1:8080/readyz >/dev/null
