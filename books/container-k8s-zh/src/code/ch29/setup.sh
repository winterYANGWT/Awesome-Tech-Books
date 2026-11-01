#!/usr/bin/env bash
# 第 29 章的起点：3 节点 kind 集群 + s3-stateful。
# api 已经带上 requests/limits（第 28 章的结果），但集群里还没有 Metrics API。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s3-stateful kind-3n.yaml

# 第 28 章结束时 api 的资源声明；本章的 HPA 用 requests.cpu 作分母。
"${KUBECTL[@]}" -n shorty patch deployment api --type=strategic -p \
  '{"spec":{"template":{"spec":{"containers":[{"name":"api","resources":{"requests":{"cpu":"45m","memory":"64Mi"},"limits":{"cpu":"500m","memory":"256Mi"}}}]}}}}' \
  >/dev/null
"${KUBECTL[@]}" -n shorty rollout status deployment/api --timeout=180s >/dev/null

# 示例里要用到的镜像先放进 lab，避免输出混进下载进度。
pull_images registry.k8s.io/metrics-server/metrics-server:v0.9.0 \
  registry.k8s.io/autoscaling/vpa-recommender:1.8.1
docker build -q -t shorty-loadgen:0.1 loadgen >/dev/null
_kind_load_image shorty-loadgen:0.1
