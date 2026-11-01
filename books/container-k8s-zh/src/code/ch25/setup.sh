#!/usr/bin/env bash
# 第 25 章的起点：s3-stateful 已经跑起来，Service 可用，集群里没有任何入口控制器。
# 提前把路径路由要用的前端镜像和 ingress2gateway 准备好，示例输出里就不会混进下载进度。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s3-stateful kind-1n.yaml
pull_images nginx:1.29-alpine

# ingress2gateway v1.2.0：25.6 用它把 Ingress 转成 Gateway/HTTPRoute。
if ! command -v ingress2gateway >/dev/null 2>&1; then
  tmp=$(mktemp -d)
  curl -fsSL --max-time 180 -o "$tmp/i2g.tar.gz" \
    https://github.com/kubernetes-sigs/ingress2gateway/releases/download/v1.2.0/ingress2gateway_Linux_x86_64.tar.gz
  tar -xzf "$tmp/i2g.tar.gz" -C "$tmp" --no-same-owner ingress2gateway
  install -m 0755 "$tmp/ingress2gateway" /usr/local/bin/ingress2gateway
  rm -rf "$tmp"
fi
ingress2gateway version
