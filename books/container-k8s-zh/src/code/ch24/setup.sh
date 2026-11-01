#!/usr/bin/env bash
# 第 24 章的起点：2 节点 kind 集群，带默认 kindnet；先让 s3-stateful 跑起来，
# 24.1 和 24.2 在这套网络上观察 Pod 直通和 kindnet 的策略行为。
# 24.3 会按大纲重建一个 disableDefaultCNI 的集群来装 Calico。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

export STAGE_WAIT_TIMEOUT=${STAGE_WAIT_TIMEOUT:-600}
stage_up s3-stateful kind-2n-kindnet.yaml

# 后面示例用来做连通性测试和目标 Pod。
pull_images nginx:1.29-alpine busybox:1.37

# 可选：装上 calicoctl，24.3 用它读 IPPool。没有网络时保留 kubectl 路径。
if ! command -v calicoctl >/dev/null 2>&1; then
  curl -fsSL --max-time 120 \
    https://github.com/projectcalico/calico/releases/download/v3.33.0/calicoctl-linux-amd64 \
    -o /usr/local/bin/calicoctl
  chmod +x /usr/local/bin/calicoctl
fi
