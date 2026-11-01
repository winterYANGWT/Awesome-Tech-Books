#!/usr/bin/env bash
# 第 32 章的起点：s4-prod 已就绪，另外在建集群时打开 apiserver 审计。
# 审计策略文件先放到 lab 的 /tmp，由 kind 的 extraMounts 挂进控制面节点；
# 同样的目录再挂给 apiserver 的 --audit-log-path，日志就落在节点外。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

rm -rf /tmp/ch32-audit-log
mkdir -p /tmp/ch32-audit-log
cp audit-policy.yaml /tmp/ch32-audit-policy.yaml

stage_up s4-prod kind-3n-audit.yaml

# 32.1 造暴露面、32.3 从 hostNetwork Pod 访问 kubelet、32.4 造超限 Pod、
# 32.6 跑 kube-bench 都要用这些镜像，先放进 lab 和节点，示例输出里就没有下载进度。
pull_images busybox:1.37 curlimages/curl:8.16.0 aquasec/kube-bench:v0.16.0
