#!/usr/bin/env bash
# 第 33 章的起点：s4-prod 已就绪；构建并加载本章要用的 shorty-operator 镜像。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s4-prod kind-1n.yaml

echo "==> 构建 shorty-operator:0.1"
docker build -q -t shorty-operator:0.1 -f Dockerfile.operator .
kind load docker-image --name "$KIND_CLUSTER" shorty-operator:0.1 >/dev/null
