# 轮换前：数据库接受当前密码。
kubectl -n shorty exec postgres-0 -- psql -U shorty -d shorty -tAc 'select 1'

# 先在数据库里改密码，再更新 Secret。
kubectl -n shorty exec postgres-0 -- psql -U shorty -d shorty -c "ALTER USER shorty WITH PASSWORD 'shorty-rotated-2026';"
kubectl -n shorty create secret generic shorty-db \
  --from-literal=db_password='shorty-rotated-2026' \
  --dry-run=client -o yaml | kubectl apply -f -

# 等 postgres Pod 里挂载的 Secret 文件也变成新值。
#> hide
#> timeout: 300
for i in $(seq 1 120); do
  kubectl -n shorty exec postgres-0 -- cat /etc/shorty/db_password 2>/dev/null | grep -q 'shorty-rotated-2026' && break
  sleep 2
done
kubectl -n shorty exec postgres-0 -- cat /etc/shorty/db_password

# 重启 api，让它重新读密码文件。
kubectl -n shorty rollout restart deployment/api
kubectl -n shorty rollout status deployment/api --timeout=180s

# 等新 api Pod 的启动日志出现“数据库和 Redis 已就绪”。
#> hide
#> timeout: 300
for i in $(seq 1 120); do
  POD=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api --sort-by=.metadata.creationTimestamp -o name | tail -n 1)
  kubectl -n shorty logs "$POD" --tail=-1 2>/dev/null | grep -q '数据库和 Redis 已就绪' && break
  sleep 2
done
POD=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api --sort-by=.metadata.creationTimestamp -o name | tail -n 1)
kubectl -n shorty logs "$POD" --tail=-1 | grep -E 'shorty starting|数据库和 Redis 已就绪'

# 新密码能连，旧密码连不上。
kubectl -n shorty exec postgres-0 -- sh -c 'PGPASSWORD=shorty-rotated-2026 psql -h postgres -U shorty -d shorty -tAc "select 1"'
#> fails
#> expect: password authentication failed
kubectl -n shorty exec postgres-0 -- sh -c 'PGPASSWORD=shorty-dev-password psql -h postgres -U shorty -d shorty -tAc "select 1"'
