#!/usr/bin/env bash
# 第 35 章的起点：s4-prod 已就绪（含 metrics-server、Gateway）。
# 监控与追踪组件在 35.2、35.4 的示例里安装，这样能看到安装过程和 CRD。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s4-prod kind-1n.yaml

# helm 仓库先在起点加好，示例输出里就不混进 repo update 的下载过程。
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts >/dev/null 2>&1 || true
helm repo update >/dev/null

# 本章要用的镜像先拉下来并加载进节点，避免示例输出混进拉取进度。
pull_images \
  quay.io/prometheus-operator/prometheus-operator:v0.94.2 \
  quay.io/prometheus-operator/prometheus-config-reloader:v0.94.2 \
  quay.io/prometheus/prometheus:v3.15.1-distroless \
  registry.k8s.io/kube-state-metrics/kube-state-metrics:v2.20.0 \
  ghcr.io/jkroepke/kube-webhook-certgen:1.11.0 \
  otel/opentelemetry-collector-contrib:0.155.0 \
  curlimages/curl:8.14.1 \
  alpine:3.22
