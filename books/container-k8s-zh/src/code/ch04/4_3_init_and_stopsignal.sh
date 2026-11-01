# --init 在 PID 1 注入 docker-init，shorty 变成它的子进程。
docker run -d --init --name shorty-init shorty:ch04 > /dev/null
#> expect: /sbin/docker-init -- /shorty serve
docker exec shorty-init sh -c "cat /proc/1/cmdline | tr '\\0' ' '; echo"
#> expect: docker-init
docker exec shorty-init sh -c 'ps -o pid,ppid,comm'
docker stop -t 10 shorty-init
#> expect: ExitCode=0
docker inspect -f 'ExitCode={{.State.ExitCode}}' shorty-init
docker rm -f shorty-init > /dev/null

# STOPSIGNAL SIGINT：不传 -s，docker stop 默认发的也是 SIGINT。
#> expect: shorty:ch04-int StopSignal=SIGINT
docker inspect -f '{{index .RepoTags 0}} StopSignal={{.Config.StopSignal}}' shorty:ch04 shorty:ch04-int
docker run -d --name shorty-int shorty:ch04-int > /dev/null
docker stop -t 10 shorty-int
#> expect: ExitCode=0
docker inspect -f 'ExitCode={{.State.ExitCode}}' shorty-int
#> expect: "signal":"interrupt"
docker logs shorty-int 2>&1 | grep '收到退出信号'
docker rm -f shorty-int > /dev/null
