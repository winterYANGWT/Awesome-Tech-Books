#!/usr/bin/env bash
# 第 9 章的起点：基础镜像先拉好，示例输出里就不会混进下载进度。
set -euo pipefail

source ../lib/common.sh


docker rm -f shorty-api >/dev/null 2>&1 || true

pull_images golang:1.27.3-trixie alpine:3.22 ubuntu:latest
