# 现在 api 只绑在宿主回环地址上（第 5 章的发布方式）。
docker port shorty-api
docker inspect -f '{{json .HostConfig.PortBindings}}' shorty-api

# 重建成发布到所有宿主地址。发布端口是创建时决定的，改不了，只能重建。
docker rm -f shorty-api >/dev/null
docker run -d --name shorty-api --network shorty-net --network-alias api \
    -p 8080:8080 \
    -e 'SHORTY_DB_URL=postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable' \
    -e SHORTY_REDIS_ADDR=redis:6379 \
    -e SHORTY_LISTEN=:8080 \
    shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://localhost:8080/readyz >/dev/null
curl -sS localhost:8080/healthz
docker port shorty-api
docker inspect -f '{{json .NetworkSettings.Ports}}' shorty-api

# DNAT 在宿主 nat 表里：宿主 8080 被改写到容器 IP 的 8080。
iptables -t nat -S DOCKER | grep 8080

# docker update 没有发布端口的选项，所以端口只能在创建时给。
docker update --help | grep -i publish || echo 'docker update: 没有 publish 选项'

# 改成只绑 127.0.0.1，宿主的其他地址不再可达。
docker rm -f shorty-api >/dev/null
docker run -d --name shorty-api --network shorty-net --network-alias api \
    -p 127.0.0.1:8080:8080 \
    -e 'SHORTY_DB_URL=postgres://shorty:shorty@postgres:5432/shorty?sslmode=disable' \
    -e SHORTY_REDIS_ADDR=redis:6379 \
    -e SHORTY_LISTEN=:8080 \
    shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://127.0.0.1:8080/readyz >/dev/null
docker port shorty-api

# -P 把镜像 EXPOSE 的端口随机映射到宿主高端口；这里用 --expose 补一个。
docker run -d --name ch06-random -P --expose 8080 shorty:dev /out/shorty serve >/dev/null
docker port ch06-random
random_port=$(docker port ch06-random 8080/tcp | head -1 | sed 's/.*://')
printf 'host port=%s\n' "$random_port"
curl -sS "localhost:${random_port}/healthz"
docker rm -f ch06-random >/dev/null
