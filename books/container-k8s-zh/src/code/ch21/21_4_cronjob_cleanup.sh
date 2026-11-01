# 先插入一条已经过期的链接，让 cleanup 有东西可删。
kubectl -n shorty exec -i deploy/postgres -- sh -c 'PGPASSWORD=$(cat /etc/shorty/db_password) psql -U shorty -d shorty' <<'SQL'
INSERT INTO links (code, url, expires_at)
VALUES ('expired-demo', 'http://example.com', now() - interval '1 hour')
ON CONFLICT (code) DO UPDATE SET expires_at = EXCLUDED.expires_at;
SQL

kubectl apply -f cronjob-cleanup.yaml

# 不等整分钟，先从 CronJob 模板手工创建一个 Job，验证 cleanup 本身能跑。
kubectl -n shorty create job --from=cronjob/shorty-cleanup shorty-cleanup-manual
kubectl -n shorty wait --for=condition=complete job/shorty-cleanup-manual --timeout=120s
#> expect: cleanup: 删除了 1 条过期链接
kubectl -n shorty logs job/shorty-cleanup-manual

# 再等 CronJob 自己触发一次；它创建的 Job 有 CronJob 属主。
#> timeout: 180
for i in $(seq 1 120); do
  scheduled=$(kubectl -n shorty get jobs -l app.kubernetes.io/name=shorty-cleanup -o name 2>/dev/null \
    | grep -v 'shorty-cleanup-manual' || true)
  [ -n "$scheduled" ] && break
  sleep 1
done
kubectl -n shorty wait --for=condition=complete "$scheduled" --timeout=120s

# CronJob 的调度、并发、历史策略和最近一次调度时间。
kubectl -n shorty get cronjob shorty-cleanup -o custom-columns=NAME:.metadata.name,SCHEDULE:.spec.schedule,TIMEZONE:.spec.timeZone,CONCURRENCY:.spec.concurrencyPolicy,DEADLINE:.spec.startingDeadlineSeconds,SUSPEND:.spec.suspend,LASTSCHEDULE:.status.lastScheduleTime

# 两个 Job 的属主都是 CronJob；TRIGGER 列区分手工触发和定时触发。
kubectl -n shorty get jobs -l app.kubernetes.io/name=shorty-cleanup -o json \
  | jq -r '((["NAME","TRIGGER","OWNER"]), (.items | sort_by(.metadata.name)[] | [.metadata.name, (.metadata.annotations["cronjob.kubernetes.io/instantiate"] // "scheduled"), .metadata.ownerReferences[0].kind])) | @tsv'
