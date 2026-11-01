#!/usr/bin/env bash
# 第 1 章的起点：一个本地 BusyBox 镜像（不依赖镜像仓库）和 shorty 的静态二进制。
set -euo pipefail

# 0. 清掉上次中断留下的实验 cgroup（正常情况下示例自己会删）。
if [ -d /sys/fs/cgroup/ch01demo ]; then
    for _ in $(seq 1 20); do
        echo 1 > /sys/fs/cgroup/ch01demo/cgroup.kill 2>/dev/null || true
        rmdir /sys/fs/cgroup/ch01demo 2>/dev/null || true
        [ -d /sys/fs/cgroup/ch01demo ] || break
    done
fi

# 0b. 清掉上次中断留下的 overlay 挂载和临时目录。
if mountpoint -q /tmp/ch01-ovl/merged; then
    umount -l /tmp/ch01-ovl/merged
fi
rm -rf /tmp/ch01-ovl /tmp/ch01-pivot

# 1. 用 lab 自带的 BusyBox 打一个最小 rootfs。示例只用到 sh、ps、stat、
#    netstat 这些 applet，没必要从 Docker Hub 拉镜像。
rootfs=$(mktemp -d)
trap 'rm -rf "$rootfs"' EXIT
mkdir -p "$rootfs"/bin "$rootfs"/lib "$rootfs"/proc "$rootfs"/sys "$rootfs"/dev "$rootfs"/tmp "$rootfs"/etc
cp /bin/busybox "$rootfs/bin/busybox"
cp /lib/ld-musl-x86_64.so.1 "$rootfs/lib/"
chmod 1777 "$rootfs/tmp"
for applet in $(/bin/busybox --list); do
    ln -sf busybox "$rootfs/bin/$applet"
done
tar -C "$rootfs" -cf - . | docker import -c 'CMD ["/bin/sh"]' - lab/busybox:1.37 >/dev/null

# 2. shorty 的二进制。第 4 章才正式使用 shorty，这里只借用它的 /healthz 和
#    SHORTY_FAULT_LEAK_MB 做 cgroup 实验：构建 shorty:0.1 镜像，把静态二进制拷出来。
source ../lib/common.sh
app_images
cid=$(docker create shorty:0.1)
docker cp "$cid":/shorty /tmp/ch01-shorty >/dev/null
docker rm "$cid" >/dev/null
