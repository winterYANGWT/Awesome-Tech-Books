#!/usr/bin/env bash
# 第 30 章的起点：s3-stateful 已就绪，api/worker 用的是 namespace 里的 default ServiceAccount，
# 集群里没有任何自定义的角色或绑定。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s3-stateful kind-1n.yaml

# 30.2 和 30.7 要在 Pod 里读 token、看挂载目录，先把这个小镜像放进 lab 和节点。
pull_images busybox:1.37
