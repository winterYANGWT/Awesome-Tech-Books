#!/usr/bin/env bash
# 第 8 章的起点：shorty:dev 以 root 运行所需的一切。
# shorty:dev 取多阶段 Dockerfile 的 build 阶段（带 shell 和 Go 工具链），
# 这样本章可以 exec 进去看 uid、capability 和文件权限。
set -euo pipefail

docker build -q --target build -t shorty:dev ../app/api

for _ in 1 2 3 4 5; do
    docker pull -q busybox:1.37 && break
    sleep 5
done
docker image inspect busybox:1.37 > /dev/null

# 8.4 的 rootless dockerd 和 8.7 解码 capability 用到的工具。
# 这些装在 lab 容器里，不碰宿主 daemon 的配置。
apk add -q libcap rootlesskit slirp4netns
chmod u+s /usr/bin/newuidmap /usr/bin/newgidmap
id rls > /dev/null 2>&1 || adduser -D -u 1000 rls
grep -q '^rls:' /etc/subuid || echo 'rls:231072:65536' >> /etc/subuid
grep -q '^rls:' /etc/subgid || echo 'rls:231072:65536' >> /etc/subgid
