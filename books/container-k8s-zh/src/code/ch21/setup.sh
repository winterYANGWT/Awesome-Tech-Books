#!/usr/bin/env bash
# 第 21 章的起点：s2-data 已经跑起来，包含 postgres、redis、api、worker
# 和已经完成的 Job shorty-migrate。busybox 供并行和“卡住”的 Job 使用。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s2-data
pull_images busybox:1.37
