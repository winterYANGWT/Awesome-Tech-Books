#!/usr/bin/env bash
# 第 10 章的起点：镜像先拉好，私有制品库和用于构建的 token 先准备好。
set -euo pipefail

source ../lib/common.sh

docker rm -f shorty-api shorty-worker shorty-redis ch10-artifact registry >/dev/null 2>&1 || true
docker network rm shorty-net >/dev/null 2>&1 || true

pull_images golang:1.27.3-trixie python:3.15.2-slim-trixie busybox:1.37 alpine:3.22 \
    redis:8.8.4-alpine registry:3.0.0

# 构建用的临时 token。值是演示用的，不写进正文；真实环境由 CI 的 secret 存储注入。
printf '%s' 'ch10-s3cr3t-token-7f3a' > /tmp/ch10-token

# 私有依赖打包成一个 tar.gz，由下面的制品库按 token 提供。
rm -rf /tmp/ch10-artifact
mkdir -p /tmp/ch10-artifact
tar -czf /tmp/ch10-artifact/secretlib.tar.gz -C "$PWD/private" secretlib

docker run -d --name ch10-artifact -p 8080:8080 \
    -v "$PWD/artifact_server.py:/server.py:ro" \
    -v /tmp/ch10-artifact:/srv:ro \
    -e ARTIFACT_TOKEN="$(cat /tmp/ch10-token)" \
    python:3.15.2-slim-trixie python -u /server.py >/dev/null

# 制品库起来了才继续；--retry-all-errors 比 sleep 稳定。
curl -fsS --retry 30 --retry-all-errors --retry-delay 1 \
    -H "Authorization: Bearer $(cat /tmp/ch10-token)" \
    -o /dev/null http://localhost:8080/secretlib.tar.gz
