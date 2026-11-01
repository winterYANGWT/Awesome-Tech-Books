# shorty-api 来自 4.6，SHUTDOWN_DELAY=2；先确认 readiness 正常。
#> expect: ok
curl -fsS localhost:8080/readyz

# 打开 3 秒延迟，然后发一个慢请求；这个请求会在 handler 里睡 3 秒。
curl -sS -X POST 'localhost:8080/admin/fault?latency_ms=3000' | jq -c '{latency_ms}'
curl -sS -o /tmp/ch04-slow-body -w '%{http_code} %{time_total}\n' localhost:8080/healthz > /tmp/ch04-slow.out &
CURL=$!

# 让慢请求先进入 handler，再把新请求的延迟关掉；已经进入的请求仍按 3 秒睡。
sleep 0.5
curl -sS -X POST 'localhost:8080/admin/fault?latency_ms=0' > /dev/null

# 后台 stop：SIGTERM 后 shorty 先把 readiness 置 503，等 2 秒再 Shutdown。
docker stop -t 15 shorty-api > /tmp/ch04-stop.out 2>&1 &
STOP=$!
for _ in $(seq 1 100); do
  CODE=$(curl -s -o /tmp/ch04-ready.out -w '%{http_code}' localhost:8080/readyz || true)
  [ "$CODE" = 503 ] && break
  sleep 0.05
done
#> expect: readyz during shutdown: 503
echo "readyz during shutdown: $CODE $(cat /tmp/ch04-ready.out)"
wait $STOP
wait $CURL
#> expect: slow request: 200
echo "slow request: $(cat /tmp/ch04-slow.out)"

# 日志顺序：收到信号 -> 开始关闭 -> HTTP 已关闭 -> 进程退出。
#> expect: 收到退出信号[\s\S]*开始关闭 HTTP 服务[\s\S]*HTTP 服务已关闭[\s\S]*shorty 退出
docker logs shorty-api | grep -E '收到退出信号|开始关闭 HTTP 服务|HTTP 服务已关闭|shorty 退出'
#> expect: ExitCode=0
docker inspect -f 'ExitCode={{.State.ExitCode}} OOMKilled={{.State.OOMKilled}}' shorty-api

# 重新启动，留给下一章；内存存储会从空开始。
docker start shorty-api
#> expect: ^ok$
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors localhost:8080/readyz
