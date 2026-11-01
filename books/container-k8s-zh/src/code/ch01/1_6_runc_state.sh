echo '--- runc 版本 ---'
runc --version
docker run -d --name runc-demo lab/busybox:1.37 sleep 600 >/dev/null
cid=$(docker inspect -f '{{.Id}}' runc-demo)
echo "docker inspect .Id: $cid"
echo '--- runc list（第一行是表头）---'
runc --root /run/docker/runtime-runc/moby list | head -1
runc --root /run/docker/runtime-runc/moby list | grep "$cid" | cut -c1-150

echo '--- runc 的 pivot_root 开关（默认使用 pivot_root）---'
#> expect: --no-pivot
runc run --help 2>&1 | grep -- '--no-pivot'

echo '--- runc state ---'
#> expect: "status": "running"
#> expect: "pid":
runc --root /run/docker/runtime-runc/moby state "$cid"
bundle=$(runc --root /run/docker/runtime-runc/moby state "$cid" | jq -r .bundle)
echo "bundle 目录: $bundle"
ls "$bundle"
echo '--- bundle 里 config.json 的 root 和 namespaces ---'
jq -r '.root.path' "$bundle/config.json"
#> expect: mount network time uts pid ipc cgroup
jq -r '.linux.namespaces[].type' "$bundle/config.json" | paste -sd' '
echo '--- docker inspect 的宿主 PID 与父进程 ---'
pid=$(docker inspect -f '{{.State.Pid}}' runc-demo)
echo "docker inspect .State.Pid: $pid"
ps -o pid,ppid,args -p "$pid"
ppid=$(ps -o ppid= -p "$pid" | tr -d ' ')
#> expect: containerd-shim-runc-v2
ps -o pid,ppid,args -p "$ppid" | cut -c1-120
echo '--- 这个容器的 rootfs 是一个 overlay 挂载 ---'
#> expect: type overlay
mount | grep "rootfs/overlayfs/$cid" | cut -c1-260

docker rm -f runc-demo >/dev/null
