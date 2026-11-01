#!/usr/bin/env bash
# 第 20 章的起点：3 节点集群，s1-api 的 api 1 副本。
# 本章给 api 补上 readinessProbe（滚动更新要有“新 Pod 可用”的判断依据），
# 并把 shorty:0.1 复制成 shorty:0.2，后面的滚动更新只改 tag。
set -euo pipefail

source ../lib/common.sh

stage_up s1-api kind-3n.yaml

kubectl -n shorty apply -f api-deployment.yaml >/dev/null
kubectl -n shorty rollout status deployment/api --timeout=180s >/dev/null
kubectl -n shorty annotate deployment/api kubernetes.io/change-cause="initial: 1 replica, readiness probe" --overwrite >/dev/null

# 第二个可用的镜像 tag。镜像内容与 0.1 相同，滚动更新的差别只在 tag 上。
docker tag shorty:0.1 shorty:0.2
_kind_load_image shorty:0.2

# 客户端镜像：关掉两个 api Pod 之间的请求，看滚动更新和 Recreate 的停机。
if ! docker image inspect busybox:1.37 >/dev/null 2>&1; then
    docker pull -q public.ecr.aws/docker/library/busybox:1.37 >/dev/null
    docker tag public.ecr.aws/docker/library/busybox:1.37 busybox:1.37
fi
_kind_load_image busybox:1.37
