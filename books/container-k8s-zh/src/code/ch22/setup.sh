#!/usr/bin/env bash
# 第 22 章的起点：s2-data 已经跑起来，postgres 还是 Deployment + PVC postgres-data。
# 22.1–22.3 在独立的 sts-demo 命名空间里练习 StatefulSet，22.4 才迁移 shorty 里的数据库。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s2-data
pull_images busybox:1.37
