#!/usr/bin/env bash
# 第 27 章的起点：单节点 kind 集群 + s3-stateful。
# 这时 shorty-config 和 shorty-db 已经在 shorty 命名空间里，api 通过 envFrom
# 读取 ConfigMap，并把 Secret 挂到 /etc/shorty；本章从这里检查这些用法并补上边界。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s3-stateful kind-1n.yaml
pull_images busybox:1.37
