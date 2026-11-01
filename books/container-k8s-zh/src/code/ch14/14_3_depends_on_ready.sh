# 14.3：没有 healthcheck 时，depends_on 只等“容器启动”
# 先停掉正常栈，换成只有 postgres 和 api 的 race 文件；postgres 故意晚 12 秒启动
docker compose down -v --remove-orphans

#> tail: 8
docker compose -f compose.race.yaml up -d

# api 已经在监听，但数据库还没起来：readyz 返回 503，响应体给出原因
code=000
for i in $(seq 1 40); do
  code=$(curl -s -o /tmp/ch14-ready -w '%{http_code}' localhost:8080/readyz || true)
  [ "$code" = 503 ] && break
  sleep 0.5
done
echo "status=$code"
cat /tmp/ch14-ready

# 没有 healthcheck 并不等于永远坏：api 自己每 2 秒重试，数据库起来后会恢复
code=000
for i in $(seq 1 30); do
  code=$(curl -s -o /tmp/ch14-ready -w '%{http_code}' localhost:8080/readyz || true)
  [ "$code" = 200 ] && break
  sleep 1
done
echo "status=$code"
cat /tmp/ch14-ready

# 换回带 healthcheck 的正常文件：这次 compose 会等 postgres 健康再创建 api
docker compose -f compose.race.yaml down -v
docker compose up -d
for i in $(seq 1 120); do
  if curl -fsS localhost:8080/readyz >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
docker compose ps --format json | jq -sr '.[] | [.Service, .State, (.Health // "-")] | @tsv' | sort
