# 先在宿主上留一个进程，等 unshare 退出后确认它没有受到影响。
docker run -d --name unshare-sentinel lab/busybox:1.37 sleep 600 >/dev/null
echo "宿主上的哨兵进程: PID $(docker inspect -f '{{.State.Pid}}' unshare-sentinel)"

#> expect: uid=0 gid=0
#> expect: hostname after:\s+ch01-unshare
#> expect: NSpid:\s+1
unshare --user --map-root-user --pid --fork --mount --uts --ipc --net --mount-proc \
    /bin/busybox sh -c '
        echo "uid=$(id -u) gid=$(id -g)"
        echo "hostname before: $(hostname)"
        hostname ch01-unshare
        echo "hostname after:  $(hostname)"
        echo "--- ps: 只看得到这个 PID namespace 里的进程 ---"
        ps -o pid,ppid,comm
        echo "--- /proc/1/status ---"
        grep -E "^(NSpid|CapEff)" /proc/1/status
        echo "--- 网络接口 ---"
        ip -brief link
    '

echo "退出后宿主 hostname: $(hostname)"
#> expect: 哨兵容器仍在运行: true
echo "哨兵容器仍在运行: $(docker inspect -f '{{.State.Running}}' unshare-sentinel)"
docker rm -f unshare-sentinel >/dev/null
