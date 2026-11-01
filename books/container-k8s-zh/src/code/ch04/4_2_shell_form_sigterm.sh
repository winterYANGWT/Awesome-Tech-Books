# 两个镜像的元数据：exec 形式直接以 /shorty 为入口，shell 形式包了一层 /bin/sh -c。
#> expect: Cmd=\["/bin/sh","-c","/shorty serve"\]
docker inspect -f '{{index .RepoTags 0}} Entrypoint={{json .Config.Entrypoint}} Cmd={{json .Config.Cmd}}' \
  shorty:ch04 shorty:ch04-shell

# shell 形式启动：PID 1 是 sh，shorty 是它的子进程。
docker run -d --name shorty-shell shorty:ch04-shell > /dev/null
#> expect: sh -c /shorty serve
docker exec shorty-shell sh -c "cat /proc/1/cmdline | tr '\\0' ' '; echo"

# SIGTERM 发给 PID 1 的 sh；sh 不转发，也不退出。3 秒后 Docker 发 SIGKILL。
docker stop -t 3 shorty-shell
#> expect: ExitCode=137
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-shell
#> expect: 没有 shutdown 日志
docker logs shorty-shell 2>&1 | grep '收到退出信号' || echo '没有 shutdown 日志'

# exec 形式的对照组：同一个二进制，PID 1 就是 shorty，SIGTERM 能到应用。
docker run -d --name shorty-exec shorty:ch04 > /dev/null
#> expect: /shorty serve
docker exec shorty-exec sh -c "cat /proc/1/cmdline | tr '\\0' ' '; echo"
docker stop -t 10 shorty-exec
#> expect: ExitCode=0
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-exec
docker logs shorty-exec 2>&1 | tail -n 4

docker rm -f shorty-shell shorty-exec > /dev/null
