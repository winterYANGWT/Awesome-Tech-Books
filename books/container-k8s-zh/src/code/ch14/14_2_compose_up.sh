# 14.2：构建并启动五个服务
#> tail: 20
#> timeout: 600
docker compose up -d --build --quiet-build

# 等 api 自己报告就绪，而不是猜它什么时候起来
for i in $(seq 1 120); do
  if curl -fsS localhost:8080/readyz >/dev/null 2>&1; then
    break
  fi
  sleep 1
done
curl -fsS localhost:8080/readyz

# 看每个服务的状态、健康和退出码
docker compose ps -a --format json | jq -sr '.[] | [.Service, .State, (.Health // "-"), (.ExitCode | tostring)] | @tsv' | sort

# migrate 的日志说明它做了什么
docker compose logs migrate | tail -n 6

# 命名卷被加上了项目名前缀
docker volume ls --format '{{.Name}}' | grep pgdata
docker volume inspect shorty_pgdata --format '{{.Name}} driver={{.Driver}}'
