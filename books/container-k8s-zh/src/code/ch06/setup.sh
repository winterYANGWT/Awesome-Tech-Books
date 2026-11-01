#!/usr/bin/env bash
# 第 6 章的起点：shorty-api + shorty-pg 在默认 bridge 上，pg 有第 5 章留下的表和卷，
# api 通过 pg 的容器 IP 连数据库（本章要换成服务名），还没有自定义网络。
set -euo pipefail

source ../lib/common.sh

# dev 镜像是多阶段构建里的 build 阶段：有 shell、getent 和 pg 客户端，方便 exec 进去看网络。
docker build -q --target build -t shorty:dev ../app/api
pull_images postgres:18.6-alpine redis:8.8.4-alpine
# busybox 不在 lab/preserve.txt 里；Docker Hub 限流时从 ECR Public 的官方副本拉，digest 相同。
if ! docker image inspect busybox:1.37 >/dev/null 2>&1; then
    if ! docker pull -q busybox:1.37; then
        docker pull -q public.ecr.aws/docker/library/busybox:1.37
        docker tag public.ecr.aws/docker/library/busybox:1.37 busybox:1.37
    fi
fi

docker rm -f shorty-api shorty-pg shorty-redis >/dev/null 2>&1 || true
docker network rm shorty-net >/dev/null 2>&1 || true
docker volume create shorty-pgdata >/dev/null

# 第 5 章的卷和数据库。PG 18 的 PGDATA 在 /var/lib/postgresql/18/docker，卷挂在上一层。
docker run -d --name shorty-pg \
    -e POSTGRES_USER=shorty \
    -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD=shorty \
    -v shorty-pgdata:/var/lib/postgresql \
    postgres:18.6-alpine >/dev/null

for _ in $(seq 1 60); do
    docker exec shorty-pg pg_isready -U shorty -d shorty >/dev/null 2>&1 && break
    sleep 1
done
docker exec shorty-pg pg_isready -U shorty -d shorty >/dev/null

# migrate 共享 pg 的 netns，走 localhost，不依赖名字解析。
docker run --rm --network container:shorty-pg \
    -e SHORTY_DB_URL='postgres://shorty:shorty@127.0.0.1:5432/shorty?sslmode=disable' \
    shorty:dev /out/shorty migrate

# 旧办法：默认 bridge 没有内嵌 DNS，DB 地址写成 pg 的容器 IP。
# 容器重建后 IP 会变，这正是 6.2 要替换掉的配置。
pg_ip=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' shorty-pg)
docker run -d --name shorty-api \
    -p 127.0.0.1:8080:8080 \
    -e "SHORTY_DB_URL=postgres://shorty:shorty@${pg_ip}:5432/shorty?sslmode=disable" \
    -e SHORTY_LISTEN=:8080 \
    shorty:dev /out/shorty serve >/dev/null

curl -fsS --retry 30 --retry-delay 1 --retry-all-errors \
    http://127.0.0.1:8080/readyz >/dev/null
