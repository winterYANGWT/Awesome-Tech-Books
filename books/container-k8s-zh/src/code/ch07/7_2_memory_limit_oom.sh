docker rm -f shorty-api > /dev/null

# 用 64m 内存重跑 api；先不开泄漏，等 /readyz 通过。
docker run -d --name shorty-api --network shorty-net \
    -p 8080:8080 --memory 64m \
    -v "$PWD/../app/secrets/db_password.txt:/run/secrets/db_password:ro" \
    -e SHORTY_DB_URL='postgres://shorty@shorty-pg:5432/shorty?sslmode=disable' \
    -e SHORTY_DB_PASSWORD_FILE=/run/secrets/db_password \
    -e SHORTY_REDIS_ADDR=shorty-redis:6379 \
    -e SHORTY_BASE_URL='http://localhost:8080' \
    shorty:dev /out/shorty serve > /dev/null
for _ in $(seq 1 60); do
    curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
    sleep 1
done
curl -fsS localhost:8080/readyz > /dev/null

# 把一个不占内存的 sleep 放进同一个 cgroup，容器退出后目录不会立刻被回收，
# 这样还能读到 memory.events；sleep 不参与 HTTP，不影响结果。
pid=$(docker inspect -f '{{.State.Pid}}' shorty-api)
cg=$(awk -F: '{print $3}' "/proc/$pid/cgroup")
sleep 300 &
mon=$!
echo "$mon" > "/sys/fs/cgroup$cg/cgroup.procs"

# 运行时打开故障开关，每个请求分配并保留 8 MB。
curl -fsS -X POST 'localhost:8080/admin/fault?leak_mb=8' > /dev/null
ok=0
for _ in $(seq 1 30); do
    if curl -fsS --max-time 3 -o /dev/null http://localhost:8080/; then
        ok=$((ok + 1))
    else
        break
    fi
done
echo "成功请求 $ok 次后连接断开"

#> expect: 137
docker wait shorty-api
for _ in $(seq 1 100); do
    [ "$(docker inspect -f '{{.State.OOMKilled}}' shorty-api)" = true ] && break
    sleep 0.1
done
#> expect: exited 137 true
docker inspect -f '{{.State.Status}} {{.State.ExitCode}} {{.State.OOMKilled}}' shorty-api
#> expect: oom_kill 1
cat "/sys/fs/cgroup$cg/memory.events"
kill "$mon" 2>/dev/null || true
wait "$mon" 2>/dev/null || true
rmdir "/sys/fs/cgroup$cg" 2>/dev/null || true

# 换回 128m/0.5 重启；数据在 PostgreSQL 里，不在容器的可写层。
docker rm shorty-api > /dev/null
docker run -d --name shorty-api --network shorty-net -p 8080:8080 \
    --memory 128m --memory-swap 128m --cpus 0.5 \
    -v "$PWD/../app/secrets/db_password.txt:/run/secrets/db_password:ro" \
    -e SHORTY_DB_URL='postgres://shorty@shorty-pg:5432/shorty?sslmode=disable' \
    -e SHORTY_DB_PASSWORD_FILE=/run/secrets/db_password \
    -e SHORTY_REDIS_ADDR=shorty-redis:6379 \
    -e SHORTY_BASE_URL='http://localhost:8080' \
    shorty:dev /out/shorty serve > /dev/null
for _ in $(seq 1 60); do
    curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
    sleep 1
done
curl -fsS localhost:8080/readyz > /dev/null
docker exec shorty-pg psql -U shorty -d shorty -tAc 'select count(*) from links' | sed 's/^/links=/'
code=$(docker exec shorty-pg psql -U shorty -d shorty -tAc 'select code from links order by created_at limit 1' | tr -d '[:space:]')
curl -sS -o /dev/null -w "GET /r/$code -> %{http_code}\n" "localhost:8080/r/$code"
