# 连续创建两个 migrate Job。generateName 让每次 create 得到独立的 Job 名。
first=$(kubectl -n shorty create -f job-migrate.yaml -o name)
echo "first job: $first"
kubectl -n shorty wait --for=condition=complete "$first" --timeout=120s
kubectl -n shorty logs "$first"

second=$(kubectl -n shorty create -f job-migrate.yaml -o name)
echo "second job: $second"
kubectl -n shorty wait --for=condition=complete "$second" --timeout=120s
kubectl -n shorty logs "$second"

# 两次都执行了 migrate，但表只有一张、索引也还是那两个。
#> expect: tables=1
#> expect: indexes=2
kubectl -n shorty exec -i deploy/postgres -- sh -c 'PGPASSWORD=$(cat /etc/shorty/db_password) psql -U shorty -d shorty -tA' <<'SQL'
SELECT 'tables=' || count(*) FROM pg_tables WHERE schemaname = 'public' AND tablename = 'links';
SELECT 'indexes=' || count(*) FROM pg_indexes WHERE schemaname = 'public' AND tablename = 'links';
SQL

kubectl -n shorty get jobs -l app.kubernetes.io/name=shorty-migrate-check
