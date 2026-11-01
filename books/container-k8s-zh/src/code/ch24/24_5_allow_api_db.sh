# 24.5 DNS 的 Service 同时开 UDP 和 TCP 的 53；只放 UDP 会在响应被截断时出问题。
kubectl -n kube-system get service kube-dns

# 只加 DNS 这一条，然后重启 api：让它丢掉默认拒绝之前建立的连接。
kubectl apply -f allow-dns.yaml
kubectl -n shorty rollout restart deployment/api
kubectl -n shorty rollout status deployment/api --timeout=120s

# worker 现在能解析 postgres 的名字；解析走 kube-system 里的 DNS。
dns_ok=0
for i in $(seq 1 20); do
  ip=$(timeout 10 kubectl -n shorty exec deploy/worker -- \
    python -c 'import socket; print(socket.gethostbyname("postgres"))' 2>/dev/null) || true
  if [ -n "$ip" ]; then
    printf 'worker resolves postgres to %s\n' "$ip"
    dns_ok=1
    break
  fi
  sleep 1
done
[ "$dns_ok" -eq 1 ]

# 只放行 api -> postgres:5432。
kubectl apply -f allow-api-db.yaml

# worker 不在上一条策略的源里，它到 5432 仍然是默认拒绝。
blocked=0
if timeout 12 kubectl -n shorty exec deploy/worker -- \
  python -c 'import socket; socket.create_connection(("postgres", 5432), timeout=3)' >/dev/null 2>&1; then
  echo "unexpected: worker 仍能连 postgres"
else
  echo "blocked: worker -> postgres:5432 超时"
  blocked=1
fi
[ "$blocked" -eq 1 ]

# 不经过 Ingress，从节点侧 port-forward 到 api，单独验证 api 自己的依赖。
kubectl -n shorty port-forward deployment/api 18080:8080 >/tmp/ch24-port-forward.log 2>&1 &
pf_pid=$!

# api 的 /readyz 同时 ping PostgreSQL 和 Redis。现在 DB 已放行、Redis 还没有，
# 所以这里应当只报 redis；这一步也证明 api -> postgres 已通。
redis_blocked=0
for i in $(seq 1 30); do
  body=$(curl -sS --max-time 8 --retry 2 --retry-connrefused --retry-delay 1 \
    http://127.0.0.1:18080/readyz 2>/dev/null || true)
  case "$body" in
    *database*)
      ;;
    *redis*)
      printf '%s\n' "$body"
      redis_blocked=1
      break
      ;;
  esac
  sleep 1
done
[ "$redis_blocked" -eq 1 ]

# 放行 api/worker 到 redis:6379；api 还需要写队列。
kubectl apply -f allow-worker-redis.yaml

ready=0
for i in $(seq 1 20); do
  body=$(curl -sS --max-time 8 --retry 2 --retry-connrefused --retry-delay 1 \
    http://127.0.0.1:18080/readyz 2>/dev/null || true)
  if [ "$body" = "ok" ]; then
    printf '%s\n' "$body"
    ready=1
    break
  fi
  sleep 1
done
[ "$ready" -eq 1 ]

# worker 到 redis 也走同一条策略。
redis_ok=0
for i in $(seq 1 20); do
  out=$(timeout 10 kubectl -n shorty exec deploy/worker -- \
    python -c 'import socket; socket.create_connection(("redis", 6379), timeout=3); print("connected")' \
    2>/dev/null) || true
  if [ "$out" = "connected" ]; then
    printf 'worker -> redis:6379 connected\n'
    redis_ok=1
    break
  fi
  sleep 1
done
[ "$redis_ok" -eq 1 ]

# 最后把 worker -> postgres 也加到 allow-api-db 里，再验证一次。
kubectl apply -f allow-api-db-worker.yaml
worker_db=0
for i in $(seq 1 20); do
  if timeout 12 kubectl -n shorty exec deploy/worker -- \
    python -c 'import socket; socket.create_connection(("postgres", 5432), timeout=3)' >/dev/null 2>&1; then
    echo "worker -> postgres:5432 connected"
    worker_db=1
    break
  fi
  sleep 1
done
[ "$worker_db" -eq 1 ]

kill "$pf_pid" 2>/dev/null || true
wait "$pf_pid" 2>/dev/null || true

# 放行 ingress 命名空间里的 Gateway 到 api:8080。
kubectl apply -f allow-gateway.yaml
kubectl -n shorty get networkpolicy
