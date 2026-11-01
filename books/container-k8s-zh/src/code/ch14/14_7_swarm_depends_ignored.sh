# 14.7：Compose 栈先停掉，把同一份依赖关系改写成 legacy v3 文件交给 Swarm
docker compose down -v --remove-orphans

# 单节点也能开 swarm mode；这一步只影响当前 lab 的 Docker
#> head: 4
docker swarm init --advertise-addr eth0

# -d 立刻返回；--resolve-image never 表示用本机已有的 shorty:0.1，不去 registry 查摘要
docker stack deploy -d --resolve-image never -c compose.swarm.yaml shorty-swarm

# postgres 还在 sleep 12 秒，api 已经启动；readyz 直接给出连接失败
code=000
for i in $(seq 1 40); do
  code=$(curl -s -o /tmp/ch14-swarm-ready -w '%{http_code}' localhost:8080/readyz || true)
  [ "$code" = 503 ] && break
  sleep 0.5
done
echo "status=$code"
cat /tmp/ch14-swarm-ready

# 两个 task 都是 Running，但时间戳说明 api 先开始监听，postgres 后 ready
docker service ps shorty-swarm_api --format '{{.Name}} {{.CurrentState}}'
docker service ps shorty-swarm_postgres --format '{{.Name}} {{.CurrentState}}'
docker service logs --timestamps shorty-swarm_api > /tmp/ch14-swarm-api.log 2>&1 || true
grep 'HTTP 服务开始监听' /tmp/ch14-swarm-api.log | head -n 1
for i in $(seq 1 40); do
  docker service logs shorty-swarm_postgres 2>&1 | grep -q 'database system is ready' && break
  sleep 1
done
docker service logs --timestamps --tail 200 shorty-swarm_postgres > /tmp/ch14-swarm-postgres.log 2>&1 || true
#> expect: database system is ready
grep 'database system is ready' /tmp/ch14-swarm-postgres.log | head -n 1

# 清理：先删 stack，再退出 swarm，避免影响下一次 lab 运行
docker stack rm shorty-swarm
for i in $(seq 1 30); do
  [ -z "$(docker stack services shorty-swarm -q 2>/dev/null)" ] && break
  sleep 1
done
docker swarm leave --force
docker info --format 'swarm={{.Swarm.LocalNodeState}}'
