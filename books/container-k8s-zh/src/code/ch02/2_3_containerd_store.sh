# containerd image store：存储驱动是 overlayfs snapshotter，而不是经典 overlay2 graph driver。
#> expect: io.containerd.snapshotter.v1
docker info --format 'Server Version: {{.ServerVersion}}
Storage Driver: {{.Driver}}
Driver Status: {{json .DriverStatus}}
'

# Docker 29 的 image ls 列：IMAGE / ID / DISK USAGE / CONTENT SIZE / EXTRA。
#> expect: DISK USAGE
docker image ls | sed -n '1p;/^alpine/p;/^busybox/p;/^shorty/p;/^ch02/p'

# shorty:0.1 是单平台镜像，--tree 只有一行。
#> expect: shorty:0.1
docker image ls --tree shorty:0.1

# Docker 29 没有 docker image mount 子命令；image 子命令只有这些。
#> expect: Commands:
docker image --help | sed -n '/Commands:/,/^$/p'

# 只读挂载镜像内容的做法是 docker run --mount type=image，且要求 containerd image store。
#> expect: busybox
#> head: 12
docker run --rm --mount type=image,src=busybox:1.37,dst=/dbg alpine:3.22 ls /dbg/bin
