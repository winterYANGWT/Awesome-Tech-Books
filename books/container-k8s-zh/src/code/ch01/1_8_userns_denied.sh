echo '--- 宿主上的 userns 开关 ---'
cat /proc/sys/kernel/unprivileged_userns_clone
sysctl -n kernel.apparmor_restrict_unprivileged_userns

echo '--- 宿主 root 执行 unshare --user --map-root-user ---'
unshare --user --map-root-user id -u
echo '--- /etc/subuid（没有 root 的条目）---'
cat /etc/subuid

echo '--- 容器默认的 seccomp 模式 ---'
#> expect: Seccomp:\s+2
docker run --rm lab/busybox:1.37 grep -E 'Seccomp|CapEff' /proc/self/status

echo '--- 用拒绝 unshare 的 seccomp profile 复现 EPERM ---'
#> fails
#> expect: Operation not permitted
docker run --rm --security-opt seccomp=seccomp-deny-unshare.json \
    lab/busybox:1.37 unshare --user --map-root-user id -u

echo '--- 同一镜像，放开 seccomp 并加 SYS_ADMIN 后成功 ---'
#> expect: ^0$
docker run --rm --cap-add=SYS_ADMIN --security-opt seccomp=unconfined \
    lab/busybox:1.37 unshare --user --map-root-user id -u
