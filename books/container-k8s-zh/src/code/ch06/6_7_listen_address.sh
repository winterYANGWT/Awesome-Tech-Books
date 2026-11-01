# api 监听 127.0.0.1:8080：端口发布还在，但从别的容器访问 api:8080 会被拒绝。
docker rm -f shorty-api >/dev/null
docker run -d --name shorty-api --network shorty-net --network-alias api \
    -p 127.0.0.1:8080:8080 \
    -e 'SHORTY_DB_URL=postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable' \
    -e SHORTY_REDIS_ADDR=redis:6379 \
    -e SHORTY_LISTEN=127.0.0.1:8080 \
    shorty:dev /out/shorty serve >/dev/null
for _ in $(seq 1 30); do
    docker logs shorty-api 2>&1 | grep -q 'HTTP 服务开始监听' && break
    sleep 1
done
docker logs shorty-api 2>&1 | grep 'HTTP 服务开始监听'

# 先分开确认三件事：DNS 能解析、路由有默认网关、端口确实拒绝连接。
#> expect: 1 packets received
docker run --rm --network shorty-net busybox:1.37 ping -c 1 -W 2 api
docker run --rm --network shorty-net busybox:1.37 ip route
#> fails
#> expect: Connection refused
docker run --rm --network shorty-net busybox:1.37 wget -qO- -T 3 http://api:8080/healthz

# 进入 api 的 netns 看监听地址：ss 显示只有 127.0.0.1:8080，没有 0.0.0.0。
pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
nsenter -t "$pid" -n ss -lntp | grep 8080
nsenter -t "$pid" -n ip route

# 修复：监听所有地址，重建容器（环境变量同样只能在创建时给）。
docker rm -f shorty-api >/dev/null
docker run -d --name shorty-api --network shorty-net --network-alias api \
    -p 127.0.0.1:8080:8080 \
    -e 'SHORTY_DB_URL=postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable' \
    -e SHORTY_REDIS_ADDR=redis:6379 \
    -e SHORTY_LISTEN=:8080 \
    shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://127.0.0.1:8080/readyz >/dev/null
docker run --rm --network shorty-net busybox:1.37 wget -qO- -T 3 http://api:8080/healthz
