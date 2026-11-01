# 默认 bridge 上的 api：/etc/resolv.conf 是宿主 resolv.conf 的拷贝，没有 127.0.0.11。
docker exec shorty-api sh -c 'cat /etc/resolv.conf'
docker exec shorty-api sh -c 'getent hosts postgres; echo exit=$?'
docker exec shorty-api sh -c 'grep -c postgres /etc/hosts || true'

# 自定义 bridge：Docker 在 127.0.0.11 起内嵌 DNS，服务名和别名由它解析。
docker network create shorty-net
docker run -d --name shorty-redis --network shorty-net --network-alias redis redis:8.8.4-alpine >/dev/null
docker network connect --alias postgres shorty-net shorty-pg
docker network connect --alias api shorty-net shorty-api
docker network inspect shorty-net --format '{{range .Containers}}{{.Name}}{{"\n"}}{{end}}' | sort

# 接入自定义网络后 resolv.conf 指向 127.0.0.11，别名 postgres 能解析。
docker exec shorty-api sh -c 'cat /etc/resolv.conf'
docker exec shorty-api sh -c 'getent hosts postgres'

# 把 pg 从网络里断开，同一个名字立刻解析失败；再接回去。
docker network disconnect shorty-net shorty-pg
docker exec shorty-api sh -c 'getent hosts postgres; echo exit=$?'
docker network connect --alias postgres shorty-net shorty-pg
docker exec shorty-api sh -c 'getent hosts postgres'
