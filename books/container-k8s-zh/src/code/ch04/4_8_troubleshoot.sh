# 故障一：shell 形式启动。先看 PID 1 是谁，再用默认的 10 秒 stop。
docker run -d --name shorty-broken shorty:ch04-shell > /dev/null
#> expect: sh -c /shorty serve
docker exec shorty-broken sh -c "cat /proc/1/cmdline | tr '\\0' ' '; echo"
START=$(date +%s%N)
docker stop shorty-broken
#> expect: waited 10s
echo "waited $(( ($(date +%s%N) - START) / 1000000000 ))s"
#> expect: ExitCode=137
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-broken
docker logs shorty-broken 2>&1 | grep '收到退出信号' || echo '没有 shutdown 日志'

# 镜像元数据也能看出问题：Cmd 是 /bin/sh -c，Entrypoint 为空。
#> expect: Cmd=\["/bin/sh","-c","/shorty serve"\]
docker inspect -f 'Entrypoint={{json .Config.Entrypoint}} Cmd={{json .Config.Cmd}}' shorty-broken
docker rm -f shorty-broken > /dev/null

# 修复：换成 exec 形式，PID 1 直接是 /shorty，stop 能正常结束。
docker run -d --name shorty-fixed shorty:ch04 > /dev/null
#> expect: /shorty serve
docker exec shorty-fixed sh -c "cat /proc/1/cmdline | tr '\\0' ' '; echo"
docker stop -t 10 shorty-fixed
#> expect: ExitCode=0
docker inspect -f 'ExitCode={{.State.ExitCode}}' shorty-fixed
docker rm -f shorty-fixed > /dev/null

# 故障二：日志驱动设成 none，docker logs 读不到；LogPath 也是空的。
docker run -d --name shorty-silent --log-driver none shorty:ch04 > /dev/null
#> fails
#> expect: configured logging driver does not support reading
docker logs shorty-silent
#> expect: driver=none path=$
docker inspect -f 'driver={{.HostConfig.LogConfig.Type}} path={{.LogPath}}' shorty-silent
docker rm -f shorty-silent > /dev/null
