#!/usr/bin/env bash
# 第 36 章的起点：s4-prod 已就绪，3 节点（1 控制面 + 2 worker）。
# etcd 跑在控制面节点的静态 Pod 里；本章的维护对象是节点、证书和 etcd 数据目录。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s4-prod kind-3n.yaml

# 36.3、36.7 的演示 Pod 用这个小镜像，先预拉并加载进节点。
pull_images alpine:3.22
