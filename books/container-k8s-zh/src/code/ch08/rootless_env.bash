#!/usr/bin/env bash
# 8.4 用的 rootless dockerd 启停函数。
# 这个 lab 是嵌套的 dind：rootlesskit 需要把 /run 换成自己的 tmpfs，
# 而外层 dockerd 的 /run/docker 不能被覆盖，所以先在私有 mount namespace 里
# 准备一个可写的 /run，再让 rls 用户启动 rootlesskit。
# 这是实验环境的适配，不是生产上的 rootless 安装步骤。

RLS_USER=rls
RLS_BASE=/tmp/ch08-rootless
RLS_SOCK=$RLS_BASE/docker.sock

rootless_start() {
    local old_userns old_port
    old_userns=$(cat /proc/sys/kernel/apparmor_restrict_unprivileged_userns)
    old_port=$(cat /proc/sys/net/ipv4/ip_unprivileged_port_start)
    printf '%s %s\n' "$old_userns" "$old_port" > "$RLS_BASE.sysctl"

    rm -rf "$RLS_BASE" "$RLS_BASE-data" "$RLS_BASE-exec" "$RLS_BASE.log"
    mkdir -p "$RLS_BASE" "$RLS_BASE-data" "$RLS_BASE-exec"
    chown -R "$RLS_USER:$RLS_USER" "$RLS_BASE" "$RLS_BASE-data" "$RLS_BASE-exec"

    # 这个内核默认禁止非特权用户创建带 root 映射的 user namespace；
    # 只在本例启动 daemon 的几秒内放开，启动完立刻恢复。
    sysctl -qw kernel.apparmor_restrict_unprivileged_userns=0
    # 宿主把 1024 以下端口也开放给普通用户；恢复成 1024 才能看到
    # rootless 的端口限制。这个 sysctl 属于 lab 的 network namespace。
    sysctl -qw net.ipv4.ip_unprivileged_port_start=1024

    unshare --mount --propagation private sh -c '
      mount -t tmpfs tmpfs /run
      mkdir -p /run/docker
      chown 1000:1000 /run/docker
      su -s /bin/sh rls -c "
        export XDG_RUNTIME_DIR=/run/docker DOCKER_IGNORE_BR_NETFILTER_ERROR=1
        nohup rootlesskit --state-dir=/run/docker/rootlesskit \
          --net=slirp4netns --mtu=65520 --disable-host-loopback \
          --port-driver=builtin --pidns \
          dockerd --rootless --iptables=false --ip6tables=false \
            --data-root=/tmp/ch08-rootless-data \
            --exec-root=/tmp/ch08-rootless-exec \
            --pidfile=/run/docker/dockerd.pid \
            --host=unix:///tmp/ch08-rootless/docker.sock \
            --userland-proxy=false > /tmp/ch08-rootless.log 2>&1 &
      "
    '

    local i
    for i in $(seq 1 90); do
        DOCKER_HOST="unix://$RLS_SOCK" docker info > /dev/null 2>&1 && break
        sleep 1
    done
    sysctl -qw kernel.apparmor_restrict_unprivileged_userns="$old_userns"
    if ! DOCKER_HOST="unix://$RLS_SOCK" docker info > /dev/null 2>&1; then
        rootless_stop
        return 1
    fi
}

rootless_stop() {
    pkill -f 'rootlesskit.*ch08-rootless' 2> /dev/null || true
    pkill -f 'dockerd --rootless.*ch08-rootless' 2> /dev/null || true
    sleep 1
    if [ -f "$RLS_BASE.sysctl" ]; then
        read -r u p < "$RLS_BASE.sysctl" || true
        [ -n "${u:-}" ] && sysctl -qw kernel.apparmor_restrict_unprivileged_userns="$u" || true
        [ -n "${p:-}" ] && sysctl -qw net.ipv4.ip_unprivileged_port_start="$p" || true
    fi
    rm -rf "$RLS_BASE" "$RLS_BASE-data" "$RLS_BASE-exec"
}
