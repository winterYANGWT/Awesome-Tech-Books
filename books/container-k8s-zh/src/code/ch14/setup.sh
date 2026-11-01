#!/usr/bin/env bash
# 第 14 章的起点：Compose 项目要用的镜像都先构建/拉好，示例里就不会混进构建和下载进度。
set -euo pipefail

source ../lib/common.sh

# 上一次运行如果留下 swarm，先退出，避免影响后面的 stack deploy 示例。
docker swarm leave --force >/dev/null 2>&1 || true
docker rm -f ch14-port-hog >/dev/null 2>&1 || true

# lab 里同时跑多个任务时，inotify 实例可能不够，compose watch 会报
# "hit OS limits creating a watcher"；调大上限只影响这个 lab。
sysctl -w fs.inotify.max_user_instances=512 >/dev/null 2>&1 || true

app_images
pull_images postgres:18.6-alpine redis:8.8.4-alpine nginx:1.29-alpine python:3.15.2-slim-trixie

# 预构建 Compose 里的自定义镜像，示例只验证启动和依赖关系。
docker compose -f compose.yaml -f compose.watch.yaml --profile watch --profile tools build -q
