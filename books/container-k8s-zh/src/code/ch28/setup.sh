#!/usr/bin/env bash
# 第 28 章的起点：3 节点 kind 集群 + s3-stateful。
# 两个 worker 打上 shorty.example.com/role=worker，供拓扑分布示例使用。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s3-stateful kind-3n.yaml
pull_images alpine:3.22
kubectl label node shorty-worker shorty-worker2 shorty.example.com/role=worker --overwrite
