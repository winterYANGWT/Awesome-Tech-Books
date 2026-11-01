# 3.2 手工准备一个 OCI runtime bundle：config.json + rootfs
# 从 busybox 镜像导出 rootfs。busybox 已由 setup.sh 拉好；docker create 只是为了拿到一个可 export 的容器。
rm -rf /tmp/ch03-runc
mkdir -p /tmp/ch03-runc/rootfs
cid=$(docker create busybox:1.37)
docker export "$cid" | tar -C /tmp/ch03-runc/rootfs -xf -
docker rm "$cid" >/dev/null
ls /tmp/ch03-runc/rootfs

cd /tmp/ch03-runc
runc spec

# 只看本章关心的字段，不把整个 config.json 打出来。
jq '{args: .process.args, terminal: .process.terminal,
     root: .root,
     namespaces: [.linux.namespaces[].type],
     cgroupsPath: .linux.cgroupsPath}' config.json

# 先做一次前台运行：runc run 等于 create + start + 等进程退出 + delete。
jq '.process.args=["/bin/echo","hello from runc"] | .process.terminal=false' \
  config.json > config.tmp && mv config.tmp config.json
runc run ch03-oneshot
runc list

# 再做分离式：create 只创建，start 才启动；中间可以看状态。
jq '.process.args=["/bin/sleep","60"] | .process.terminal=false
    | .linux.cgroupsPath="ch03-runc"' config.json > config.tmp && mv config.tmp config.json
runc create ch03-runc
runc state ch03-runc | jq '{id, pid, status, bundle, rootfs}'

runc start ch03-runc
runc state ch03-runc | jq '{id, pid, status, bundle, rootfs}'
runc exec ch03-runc /bin/echo inside-runc

# kill 之后先别 delete：state 会变成 stopped。delete 之后连记录都没有了。
runc kill ch03-runc KILL
runc state ch03-runc | jq '{id, pid, status}'
runc delete ch03-runc
runc list

cd /
rm -rf /tmp/ch03-runc
