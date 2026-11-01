#!/usr/bin/env bash
# 第 2 章的起点：基础镜像 + shorty 镜像。第一次运行会下载 Go 模块。
set -euo pipefail

CACHE=/book/build/scratch/ch02/cache/images.tar

# 本 lab 访问 Docker Hub 经常被限流；用 Docker 官方镜像在 ECR Public 的副本，
# 拉回来后仍打成 alpine:3.22 / busybox:1.37。镜像 digest 与 Docker Hub 一致。
pull_public() {
    local name=$1 dest=$2 platform=${3:-}
    local src="public.ecr.aws/docker/library/$name"
    local i
    for i in 1 2 3; do
        if docker pull -q ${platform:+--platform "$platform"} "$src"; then
            [ -n "$dest" ] && docker tag "$src" "$dest"
            return 0
        fi
        sleep 3
    done
    return 1
}

if docker image inspect alpine:3.22 busybox:1.37 >/dev/null 2>&1; then
    :
elif [ -f "$CACHE" ]; then
    docker load -q -i "$CACHE"
else
    pull_public alpine:3.22 alpine:3.22 linux/amd64
    pull_public alpine:3.22 "" linux/arm64
    docker tag public.ecr.aws/docker/library/alpine:3.22 alpine:3.22
    pull_public busybox:1.37 busybox:1.37 linux/amd64
    mkdir -p "$(dirname "$CACHE")"
    docker save alpine:3.22 busybox:1.37 -o "$CACHE"
fi

# 生产镜像：distroless 基础层 + 复制进去的 shorty 二进制。
docker build -q -t shorty:0.1 ../app/api

# 开发用的工具镜像：直接取多阶段构建里的 build 阶段，带 shell 和 Go 工具链。
docker build -q --target build -t shorty:dev ../app/api
