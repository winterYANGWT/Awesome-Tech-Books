#!/usr/bin/env bash
# 先看存储驱动和数据根目录，再看四类占用。
docker info --format 'Storage Driver={{.Driver}} Docker Root Dir={{.DockerRootDir}}'
docker system df

# 三次不带 tag 的构建，留下三个悬空镜像；再留两个停止的容器。
for pad in 2 4 6; do
    docker build -q --build-arg PAD="$pad" -f disk.Dockerfile . > /dev/null
done
docker build -q -t ch05-disk:1 --build-arg PAD=8 -f disk.Dockerfile . > /dev/null
docker run --name ch05-stopped-1 shorty:dev sh -c 'dd if=/dev/zero of=/filler bs=1M count=8' > /dev/null 2>&1
docker run --name ch05-stopped-2 shorty:dev sh -c 'dd if=/dev/zero of=/filler bs=1M count=8' > /dev/null 2>&1

# 制造之后：容器行、镜像行和构建缓存都应该变大。
docker system df
docker ps -a --filter name=ch05-stopped --format 'table {{.Names}}\t{{.Status}}'
docker image ls --filter dangling=true --format 'table {{.Repository}}\t{{.Tag}}\t{{.ID}}'

# system prune 不碰卷；只清停止容器、悬空镜像和构建缓存。
#> expect: Total reclaimed space
docker system prune -f | grep -E '^(Deleted|Total reclaimed)'

# 命名卷还在，带 tag 的 ch05-disk:1 也还在，说明 prune 默认不删它们。
docker volume ls --filter name=shorty --format 'table {{.Driver}}\t{{.Name}}'
docker image ls ch05-disk
docker rmi ch05-disk:1 > /dev/null

# 清理后的四类占用对比。
docker system df
