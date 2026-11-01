# 让 shorty 在 2 秒后以 1 退出，制造一个已退出的容器。
docker run -d --name shorty-crash -e SHORTY_FAULT_CRASH_AFTER=2 shorty:ch04 > /dev/null
docker wait shorty-crash
#> hide
for _ in $(seq 1 50); do [ "$(docker inspect -f '{{.State.Status}}' shorty-crash)" = exited ] && break; sleep 0.05; done
#> expect: Exited \(1\)
docker ps -a --filter name=shorty-crash --format 'table {{.Names}}\t{{.Status}}'

# 容器退出后，docker logs 仍能读到退出前的 JSON 日志。
#> expect: 故障开关触发退出
docker logs shorty-crash
# 日志驱动和路径都在 HostConfig/LogPath 里。
#> expect: driver=json-file
docker inspect -f 'driver={{.HostConfig.LogConfig.Type}} config={{json .HostConfig.LogConfig.Config}} path={{.LogPath}}' shorty-crash

# json-file 的每一行是 log/stream/time 三个字段。
LOG=$(docker inspect -f '{{.LogPath}}' shorty-crash)
#> expect: "stream": "stdout"
tail -n 1 "$LOG" | jq '{log, stream, time}'

# 读取方式：--tail 限行数，-t 加时间戳，--since 限时间窗口。
docker logs --tail 2 shorty-crash
#> expect: ^[0-9]{4}-[0-9]{2}-[0-9]{2}T
docker logs -t --tail 2 shorty-crash | cut -c1-120
docker logs --since 10m --tail 2 shorty-crash

# max-size/max-file 让 json-file 轮转；这里只保留 2 个 1k 的文件。
docker run -d --name shorty-rotate --log-opt max-size=1k --log-opt max-file=2 busybox:1.37 \
  sh -c 'i=0; while [ $i -lt 300 ]; do echo "line $i 012345678901234567890123456789"; i=$((i+1)); done' > /dev/null
docker wait shorty-rotate
#> expect: max-file
docker inspect -f '{{json .HostConfig.LogConfig.Config}}' shorty-rotate
#> expect: -json.log
find "$(dirname "$(docker inspect -f '{{.LogPath}}' shorty-rotate)")" -maxdepth 1 -name '*-json.log*' -printf '%f\n' | sort

docker rm -f shorty-crash shorty-rotate > /dev/null
