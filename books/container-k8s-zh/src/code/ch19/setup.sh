#!/usr/bin/env bash
# 第 19 章的起点：s1-api 的集群，加上本章示例要用的 busybox。
set -euo pipefail
source ../lib/common.sh

# kind 在共享 lab 里偶尔会碰上 kubeadm 初始化超时；重试一次，避免把环境抖动
# 当成章节示例的问题。第二次仍失败就按正常错误退出。
if ! stage_up s1-api; then
    echo "==> stage_up 失败，重试一次"
    stage_up s1-api
fi
pull_images busybox:1.37
