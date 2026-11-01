#!/usr/bin/env bash
# 第 11 章的起点：第 10 章留下的 scratch 版 shorty:0.1、shorty-worker:0.1，
# 外加本章要比较和扫描用的基础镜像、Trivy 与它的漏洞库。
set -euo pipefail

source ../lib/common.sh

docker rm -f shorty-api shorty-worker ch11-registry >/dev/null 2>&1 || true
docker network rm shorty-net >/dev/null 2>&1 || true

pull_images golang:1.27.3-trixie python:3.15.2-slim-trixie alpine:3.22 busybox:1.37 \
    registry:3.0.0 aquasec/trivy:0.76.1

# 起点状态的 shorty:0.1：scratch、非 root、version 0.1，还没有 OCI 标签。
docker buildx build -q --load --target minimal -f base-compare.Dockerfile \
    -t shorty:0.1 ../app/api >/dev/null

# worker 用 python slim，保留它的 shell 和 apt，留到 11.1、11.4 做对照。
docker build -q -t shorty-worker:0.1 ../app/worker >/dev/null

# 先把 Trivy 的漏洞库下载到命名卷，示例里就不必现场下载，输出也稳定。
docker volume create ch11-trivy-cache >/dev/null
docker run --rm -v ch11-trivy-cache:/root/.cache/trivy \
    --entrypoint trivy aquasec/trivy:0.76.1 image --download-db-only
