# 打开 500 ms 延迟，/admin/fault 本身不受影响，所以随时能关掉。
curl -fsS -X POST 'localhost:8080/admin/fault?latency_ms=500' > /dev/null
for _ in 1 2 3; do
    curl -sS -o /dev/null -w '%{http_code} %{time_total}\n' http://localhost:8080/healthz
done

# 关掉延迟，同样的请求回到毫秒级以下。
curl -fsS -X POST 'localhost:8080/admin/fault?latency_ms=0' > /dev/null
for _ in 1 2 3; do
    curl -sS -o /dev/null -w '%{http_code} %{time_total}\n' http://localhost:8080/healthz
done

# 错误率 1.0：所有业务请求都返回 500，/admin/fault 仍然可用。
curl -fsS -X POST 'localhost:8080/admin/fault?error_rate=1' > /dev/null
for _ in 1 2 3; do
    curl -sS -o /dev/null -w '%{http_code}\n' http://localhost:8080/healthz
done
curl -fsS -X POST 'localhost:8080/admin/fault?error_rate=0' > /dev/null

# 运行时打开 1 MB 泄漏，看 memory.current 的增量；随后关掉并重启清空。
pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
before=$(cat "/sys/fs/cgroup$cg/memory.current")
curl -fsS -X POST 'localhost:8080/admin/fault?leak_mb=1' > /dev/null
curl -fsS -o /dev/null http://localhost:8080/
after=$(cat "/sys/fs/cgroup$cg/memory.current")
echo "memory.current: $before -> $after (+$((after - before)) bytes)"
curl -fsS -X POST 'localhost:8080/admin/fault?leak_mb=0' > /dev/null

# 重启 api 清掉故障开关；数据在 PostgreSQL 里，应该还在。
docker restart shorty-api > /dev/null
for _ in $(seq 1 60); do
    curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
    sleep 1
done
curl -fsS localhost:8080/admin/fault
docker exec shorty-pg psql -U shorty -d shorty -tAc 'select count(*) from links' | sed 's/^/links=/'
code=$(docker exec shorty-pg psql -U shorty -d shorty -tAc 'select code from links order by created_at limit 1' | tr -d '[:space:]')
curl -sS -o /dev/null -w "GET /r/$code -> %{http_code}\n" "localhost:8080/r/$code"
