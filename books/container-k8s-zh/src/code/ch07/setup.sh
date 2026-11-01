#!/usr/bin/env bash
# 第 7 章的起点：第 6 章结束时的三个容器，都在 shorty-net 上，未设资源限制。
set -euo pipefail

# lab 刚 reset 完时 dockerd 偶尔还在收尾；等它就绪再开始。
for _ in $(seq 1 30); do
    docker info > /dev/null 2>&1 && break
    sleep 1
done

start_container() {
    local name=$1
    shift
    docker rm -f "$name" > /dev/null 2>&1 || true
    for _ in $(seq 1 5); do
        if docker run -d --name "$name" "$@" > /dev/null; then
            return 0
        fi
        sleep 1
    done
    return 1
}

run_once() {
    local i
    for i in 1 2 3 4 5; do
        docker run --rm "$@" && return 0
        sleep 1
    done
    return 1
}

# shorty:dev 带 shell 和 Go 工具链，便于从宿主机读 cgroup、从容器里看 /proc；
# shorty:0.1 用来跑一次性的 migrate。
docker build -q -t shorty:0.1 ../app/api
docker build -q --target build -t shorty:dev ../app/api

for image in postgres:18.6-alpine redis:8.8.4-alpine busybox:1.37; do
    for _ in 1 2 3 4 5; do
        docker pull -q "$image" && break
        sleep 3
    done
    docker image inspect "$image" > /dev/null
done

# 清掉上次中断留下的对象；reset 通常会做，这里只兜底，避免卷里留着旧表。
docker rm -f shorty-api shorty-pg shorty-redis > /dev/null 2>&1 || true
docker network rm shorty-net > /dev/null 2>&1 || true
docker volume rm shorty-pgdata > /dev/null 2>&1 || true

docker network create shorty-net > /dev/null
docker volume create shorty-pgdata > /dev/null

# PostgreSQL 18 的 PGDATA 是 /var/lib/postgresql/18/docker，卷挂到上一层。
start_container shorty-pg --network shorty-net \
    -v shorty-pgdata:/var/lib/postgresql \
    -v "$PWD/../app/secrets/db_password.txt:/run/secrets/db_password:ro" \
    -e POSTGRES_USER=shorty -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD_FILE=/run/secrets/db_password \
    postgres:18.6-alpine > /dev/null

for _ in $(seq 1 60); do
    docker exec shorty-pg pg_isready -U shorty -d shorty > /dev/null 2>&1 && break
    sleep 1
done
docker exec shorty-pg pg_isready -U shorty -d shorty > /dev/null

run_once --network shorty-net \
    -v "$PWD/../app/secrets/db_password.txt:/run/secrets/db_password:ro" \
    -e SHORTY_DB_URL='postgres://shorty@shorty-pg:5432/shorty?sslmode=disable' \
    -e SHORTY_DB_PASSWORD_FILE=/run/secrets/db_password \
    shorty:0.1 migrate > /dev/null

start_container shorty-redis --network shorty-net \
    redis:8.8.4-alpine redis-server --save '' --appendonly no > /dev/null
for _ in $(seq 1 30); do
    docker exec shorty-redis redis-cli ping 2>/dev/null | grep -qx PONG && break
    sleep 1
done

# 起点：api 没有任何资源限制，使用 PostgreSQL 和 Redis。
start_container shorty-api --network shorty-net -p 8080:8080 \
    -v "$PWD/../app/secrets/db_password.txt:/run/secrets/db_password:ro" \
    -e SHORTY_DB_URL='postgres://shorty@shorty-pg:5432/shorty?sslmode=disable' \
    -e SHORTY_DB_PASSWORD_FILE=/run/secrets/db_password \
    -e SHORTY_REDIS_ADDR=shorty-redis:6379 \
    -e SHORTY_BASE_URL='http://localhost:8080' \
    shorty:dev /out/shorty serve > /dev/null

for _ in $(seq 1 60); do
    curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
    sleep 1
done
curl -fsS localhost:8080/readyz > /dev/null

# 先写一条链接，后面重启 api 后用它验证数据还在。
curl -fsS -X POST localhost:8080/api/links \
    -H 'Content-Type: application/json' \
    -d '{"url":"http://shorty-api:8080/","ttl_seconds":86400}' > /dev/null

# 清掉上一次失败运行留下的 OOM 实验父 cgroup（目录为空时才能删）。
if [ -d /sys/fs/cgroup/ch07oom ]; then
    find /sys/fs/cgroup/ch07oom -depth -type d -exec rmdir {} \; 2>/dev/null || true
fi
