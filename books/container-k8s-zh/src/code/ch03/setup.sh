#!/usr/bin/env bash
# 第 3 章的起点：shorty:0.1 和 busybox:1.37 已在 lab 的 Docker 里，不建集群。
# 去掉 Dockerfile 的 syntax 指令，避免每次构建都去 registry 解析 frontend；
# 构建内容（多阶段、distroless、非 root）不变。
set -euo pipefail

sed '/^# syntax=/d' ../app/api/Dockerfile > /tmp/ch03-shorty.Dockerfile
docker build -q -f /tmp/ch03-shorty.Dockerfile -t shorty:0.1 ../app/api

# busybox 不在 lab/preserve.txt 里，每次 reset 后都要重新拉；网络抖动时重试几次。
for _ in 1 2 3 4 5; do
    docker pull -q busybox:1.37 && break
    sleep 5
done
docker image inspect busybox:1.37 > /dev/null
