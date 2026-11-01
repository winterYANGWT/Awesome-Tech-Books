#!/usr/bin/env bash
# 本章起点：单节点 kind 集群 + s1-api（Namespace shorty、ConfigMap shorty-config、
# Deployment api、Service api）。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

# lab 是多个任务共用的，宿主机忙的时候 kind 的 kubeadm 偶尔会在等待 apiserver 时超时。
# 重新建一次集群通常就好，不必把失败留给下一轮队列。
for attempt in 1 2 3 4 5; do
  if stage_up s1-api; then
    exit 0
  fi
  printf '第 %s 次 stage_up 失败，清理残留节点后重试\n' "$attempt" >&2
  docker rm -f shorty-control-plane >/dev/null 2>&1 || true
  sleep 30
done
exit 1
