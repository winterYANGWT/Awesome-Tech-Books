# 14.8：先制造一个 unhealthy 的 postgres，再制造一个 8080 端口冲突
docker compose down -v --remove-orphans

# postgres 的健康检查故意查 6543 端口，两次失败后变成 unhealthy
#> fails
#> expect: dependency failed to start
#> expect: unhealthy
docker compose -f compose.yaml -f compose.unhealthy.yaml up -d

# ps 里能直接看到 health=unhealthy；api/worker/migrate 停在 created
docker compose -f compose.yaml -f compose.unhealthy.yaml ps -a --format json | jq -sr '.[] | [.Service, .State, (.Health // "-")] | @tsv' | sort

# 健康检查命令的输出在 State.Health.Log 里，不在容器的 stdout 日志里
docker compose -f compose.yaml -f compose.unhealthy.yaml ps -q postgres | xargs docker inspect --format '{{range .State.Health.Log}}{{println .ExitCode .Output}}{{end}}' | tail -n 6

# 修复：去掉坏的覆盖文件，重新用正确的 healthcheck 起来
docker compose -f compose.yaml -f compose.unhealthy.yaml down -v
docker compose up -d
for i in $(seq 1 120); do
  if curl -fsS localhost:8080/readyz >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
curl -fsS localhost:8080/readyz

# 第二个故障：宿主 8080 已经被占用
docker compose down -v
docker run -d --name ch14-port-hog -p 8080:80 nginx:1.29-alpine >/dev/null
ss -ltn 'sport = :8080' | tail -n +2

#> fails
#> expect: port is already allocated
docker compose up -d

# 停掉占用者，清掉上一次失败留下的 api 容器，再 up
docker rm -f ch14-port-hog >/dev/null
for i in $(seq 1 30); do
  ss -ltn 'sport = :8080' | grep -q ':8080' || break
  sleep 1
done
docker compose down
docker compose up -d
for i in $(seq 1 120); do
  if curl -fsS localhost:8080/readyz >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
docker compose ps --format json | jq -sr '.[] | [.Service, .State, (.Health // "-")] | @tsv' | sort
