#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

# lab 里多个章节共享宿主机，kind 建集群偶尔会在 kubeadm 等控制面时超时；
# 多试几次通常就能过去。stage_up 每次都会先删掉同名集群。
ok=
for _ in 1 2 3 4; do
  if stage_up s1-api; then
    ok=1
    break
  fi
  sleep 10
done
[ -n "$ok" ] || exit 1

# kube-proxy / CoreDNS 比 Deployment 晚就绪；等这两个 addon 可用，
# 17.1 的输出才不会混进 0/1 或 Error 的中间状态。
"${KUBECTL[@]}" -n kube-system rollout status deployment/coredns --timeout=180s >/dev/null
"${KUBECTL[@]}" -n kube-system rollout status daemonset/kube-proxy --timeout=180s >/dev/null
