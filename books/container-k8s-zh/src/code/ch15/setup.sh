#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

# 第 14 章结束时 shorty 的镜像和 Compose 栈都在；这里重新构建镜像，
# 并把本章要用的基础镜像一次拉完，后面的示例输出里就不会混进下载进度。
app_images

# 第 14 章的 Compose 栈保持可用；镜像上面已经构建好，这里只创建容器。
docker compose -f ../app/compose.yaml up -d >/dev/null
for _ in $(seq 1 60); do
    if curl -fsS http://localhost:8080/readyz >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
curl -fsS http://localhost:8080/readyz >/dev/null

pull_images nginx:1.29-alpine postgres:18.6-alpine registry:2 \
    golang:1.27.3-trixie node:22-alpine busybox:1.37

# 本地 registry：15.4 推 CI 镜像和构建缓存，15.5 从同一个 registry 按 digest 晋升。
docker run -d --name ch15-registry -p 5000:5000 registry:2 >/dev/null
for _ in $(seq 1 60); do
    if curl -fsS http://localhost:5000/v2/ >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
curl -fsS http://localhost:5000/v2/ >/dev/null

# devcontainer CLI 不在 lab 镜像里，装进一个命名卷，版本固定到 0.89.0。
docker volume create ch15-devcontainer-cli >/dev/null
if ! docker run --rm -v ch15-devcontainer-cli:/cli node:22-alpine \
        test -f /cli/node_modules/@devcontainers/cli/devcontainer.js; then
    docker run --rm -v ch15-devcontainer-cli:/cli node:22-alpine \
        sh -c 'cd /cli && npm init -y >/dev/null 2>&1 && npm install --silent @devcontainers/cli@0.89.0'
fi

# Testcontainers 和 devcontainer 的 Go 依赖都放进同一个模块缓存卷，示例里不重复下载。
docker volume create ch15-gomod >/dev/null
docker volume create ch15-gocache >/dev/null
docker run --rm -v "$PWD/testcontainers:/src:ro" -v ch15-gomod:/go/pkg/mod \
    -w /src golang:1.27.3-trixie env GOFLAGS=-mod=readonly go mod download
docker run --rm -v "$PWD/../app/api:/workspaces/api:ro" -v ch15-gomod:/go/pkg/mod \
    -v ch15-gocache:/root/.cache/go-build -w /workspaces/api \
    -e GOFLAGS=-mod=readonly -e GOMAXPROCS=2 \
    golang:1.27.3-trixie go test -p 1 -run '^$' ./...

# devcontainer 的 build target 先构建一次，让 15.2 的 devcontainer up 复用层缓存。
docker build -q --target build -t shorty:dev ../app/api >/dev/null
