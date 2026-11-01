#!/usr/bin/env bash
# 第 26 章的起点：单节点 kind 集群 + s3-stateful。
# postgres 的 PVC data-postgres-0 已经绑定 kind 自带的默认 StorageClass standard。
# 单节点足够观察 emptyDir/PVC、回收策略和 RWOP 的单 Pod 约束。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s3-stateful kind-1n.yaml
pull_images busybox:1.37
