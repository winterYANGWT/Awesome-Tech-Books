#!/usr/bin/env bash
# 第 5 章的起点：第 4 章结束时的 shorty-api（内存存储）在跑，另备好 postgres 镜像。
set -euo pipefail

# shorty:0.1 用于一次性 migrate/pg 工具；shorty:dev 是带 shell 的开发镜像，供本章观察可写层。
docker build -q -t shorty:0.1 ../app/api
docker build -q --target build -t shorty:dev ../app/api

# postgres 镜像提前拉好，示例输出里就不会混进下载进度。
for _ in 1 2 3 4 5; do
    docker pull -q postgres:18.6-alpine && break
    sleep 5
done
docker image inspect postgres:18.6-alpine > /dev/null

# 第 4 章结束时的状态：api 用内存存储监听 8080。
docker rm -f shorty-api > /dev/null 2>&1 || true
docker run -d --name shorty-api -p 8080:8080 shorty:dev /out/shorty serve > /dev/null
for _ in $(seq 1 30); do
    curl -fsS localhost:8080/healthz > /dev/null 2>&1 && exit 0
    sleep 1
done
echo 'shorty-api 没有就绪' >&2
docker logs shorty-api >&2 || true
exit 1
