# 11.2：镜像配置里的 User，以及这个 UID 在容器里能做什么
docker image inspect --format 'User={{.Config.User}}' shorty:0.1

# 以 65532 启动 shorty，证明非 root 不影响主进程
docker run --rm shorty:0.1 version

# shorty:0.1 里没有 id，用同一 UID 的 busybox 看内核返回的身份。
# / 属于 root，65532 写不进去。
#> fails
#> expect: uid=65532
#> expect: Permission denied
docker run --rm --user 65532:65532 busybox:1.37 sh -c 'id; touch /probe'

# 只读根文件系统加一个 tmpfs 的 /tmp：写 /tmp 可以，写别处不行
docker run --rm --read-only --user 65532:65532 --tmpfs /tmp busybox:1.37 \
    sh -c 'echo ok >/tmp/shorty && cat /tmp/shorty'

#> fails
#> expect: Read-only file system
docker run --rm --read-only --user 65532:65532 busybox:1.37 \
    sh -c 'echo ok >/tmp/shorty'
