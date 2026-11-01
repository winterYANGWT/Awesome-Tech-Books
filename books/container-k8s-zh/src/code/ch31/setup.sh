#!/usr/bin/env bash
# 第 31 章的起点：s3-stateful 已就绪，api 的清单里没有 securityContext，
# shorty 命名空间也没有 Pod Security Admission 标签。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s3-stateful kind-1n.yaml

# 31.1 用一个带 shell 的 Pod 观察 uid、卷权限和只读根文件系统；
# 31.2、31.3、31.5 也用它做客户端。
pull_images busybox:1.37
