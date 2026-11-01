#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

# 本章从「Compose 栈可用、还没建过 Kubernetes 集群」开始。
# 为了让 16.1 能立刻观察控制循环，这里先建好单节点集群并把 shorty 镜像装进节点；
# 16.3 会删掉它，从零再走一遍 kind create。
cluster_up kind-1n.yaml
app_images

# Compose Bridge 的转换器是一个单独发布的 OCI 镜像；锁到 digest，避免用到不同的 latest。
docker pull -q docker/compose-bridge-kubernetes@sha256:8520c84287a5c051d6e8eba03fc51c1d7cdc08ac12d5dd2a0fb63078eb6272b4 >/dev/null
