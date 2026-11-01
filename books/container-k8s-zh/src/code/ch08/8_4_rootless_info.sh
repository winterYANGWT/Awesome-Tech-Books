# 8.4 rootless dockerd：daemon 以 rls 用户运行，容器 root 映射到 subuid
source ./rootless_env.bash
trap rootless_stop EXIT
rootless_start

# 和 rootful 的 docker info 对比：Security Options 里多出 rootless，
# 没有 systemd 委派时 Cgroup Driver 是 none。
DOCKER_HOST="unix://$RLS_SOCK" docker info 2> /dev/null | sed -n \
    -e '/^ Server Version:/p' -e '/^ Storage Driver:/p' -e '/^ Cgroup Driver:/p' \
    -e '/Security Options:/,+4p'

# daemon 进程本身的宿主 uid 是 1000（rls），不是 0。
ps -eo user=,pid=,comm=,args= | grep -E '[r]ootlesskit|[d]ockerd --rootless' | cut -c1-110

# 把宿主已有的 busybox 载入 rootless 的镜像存储；不显示下载/导入过程。
#> hide
docker save busybox:1.37 | DOCKER_HOST="unix://$RLS_SOCK" docker load > /dev/null

# rootlesskit 在宿主网络里以 uid 1000 绑定发布端口，80 是特权端口。
#> fails
#> expect: permission denied
DOCKER_HOST="unix://$RLS_SOCK" docker run --rm -p 80:8080 busybox:1.37 true

# 换成 8080 就能发布；容器本身仍以 root 运行在 user namespace 里。
DOCKER_HOST="unix://$RLS_SOCK" docker run --rm -p 18080:8080 busybox:1.37 true
