# docker create 只建容器不启动，状态是 Created。
docker create --name shorty-states -e SHORTY_SHUTDOWN_DELAY=1 shorty:ch04 > /dev/null
#> expect: Created
docker ps -a --filter name=shorty-states --format 'table {{.Names}}\t{{.Status}}\t{{.Image}}'

# start 之后进入 running；docker ps 默认只列 running。
docker start shorty-states
# 等应用真正开始监听再继续；否则 SIGTERM 可能早于 shorty 安装信号处理器。
#> hide
for _ in $(seq 1 50); do docker exec shorty-states wget -qO- http://127.0.0.1:8080/healthz > /dev/null 2>&1 && break; sleep 0.05; done
#> expect: Up
docker ps --filter name=shorty-states --format 'table {{.Names}}\t{{.Status}}'

# pause 暂停的是容器里的进程，状态变成 Paused；unpause 恢复。
docker pause shorty-states
#> expect: Paused
docker ps -a --filter name=shorty-states --format 'table {{.Names}}\t{{.Status}}'
docker unpause shorty-states

# stop 先发 SIGTERM；shorty 处理了信号，退出码 0。
docker stop shorty-states
#> expect: Exited \(0\)
docker ps -a --filter name=shorty-states --format 'table {{.Names}}\t{{.Status}}'
#> expect: "ExitCode": 0
docker inspect shorty-states --format '{{json .State}}' \
  | jq '{Status, ExitCode, OOMKilled, Error, StartedAt, FinishedAt}'

# 已退出的容器还能再 start，StartedAt 会更新。
docker start shorty-states
# 等应用真正开始监听再继续；否则 SIGTERM 可能早于 shorty 安装信号处理器。
#> hide
for _ in $(seq 1 50); do docker exec shorty-states wget -qO- http://127.0.0.1:8080/healthz > /dev/null 2>&1 && break; sleep 0.05; done
#> expect: Up
docker ps --filter name=shorty-states --format 'table {{.Names}}\t{{.Status}}'
docker rm -f shorty-states

# 143 = 128+15：进程收到 SIGTERM 后退出。这里让 shell 自己把 143 作为退出码。
docker run -d --name shorty-term busybox:1.37 \
  sh -c 'trap "exit 143" TERM; echo ready; while :; do sleep 1; done' > /dev/null
#> hide
for _ in $(seq 1 50); do docker logs shorty-term > /tmp/ch04-term.log 2>&1; grep -q '^ready$' /tmp/ch04-term.log && break; sleep 0.05; done
docker stop shorty-term
#> expect: ExitCode=143
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-term

# 137 = 128+9：SIGKILL，进程没有机会清理。
docker run -d --name shorty-kill busybox:1.37 sleep 1000 > /dev/null
docker kill -s KILL shorty-kill
#> expect: ExitCode=137
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-kill

docker rm shorty-term shorty-kill > /dev/null
