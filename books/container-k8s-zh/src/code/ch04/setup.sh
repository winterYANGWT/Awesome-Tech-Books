#!/usr/bin/env bash
# 第 4 章的起点：shorty:dev 已在 lab 的 Docker 里，不建集群。
# 从 dev 阶段派生 exec 形式和 shell 形式两个镜像，用来对比 PID 1 与信号。
set -euo pipefail

docker build -q --network=host --target build -t shorty:dev ../app/api
docker build -q --network=host -f Dockerfile -t shorty:ch04 .
docker build -q --network=host -f Dockerfile.shell -t shorty:ch04-shell .
docker build -q --network=host -f Dockerfile.stopsignal -t shorty:ch04-int .

# busybox 是 4.1 和 4.3 的对照组，不在 lab/preserve.txt 里，每次 reset 后都要重新拉。
for _ in 1 2 3 4 5; do
    docker pull -q busybox:1.37 && break
    sleep 5
done

docker image inspect shorty:dev shorty:ch04 shorty:ch04-shell shorty:ch04-int busybox:1.37 > /dev/null
