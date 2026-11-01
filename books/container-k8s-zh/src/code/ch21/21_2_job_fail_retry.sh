# 故意写错表名。backoffLimit: 2，所以会留下 3 个失败 Pod（首次 + 两次重试）。
kubectl apply -f job-fail-sql.yaml

# 等到 Job 进入 Failed；这一步包含 10s、20s 的指数退避。
#> timeout: 180
kubectl -n shorty wait --for=condition=failed job/shorty-fail-sql --timeout=150s

# Job 的终态和失败计数。
kubectl -n shorty get job shorty-fail-sql -o 'custom-columns=NAME:.metadata.name,ACTIVE:.status.active,SUCCEEDED:.status.succeeded,FAILED:.status.failed,CONDITION:.status.conditions[?(@.type=="Failed")].reason'

# 三个 Pod 都是 Failed，退出码来自 psql 的非 0 退出。
kubectl -n shorty get pods -l batch.kubernetes.io/job-name=shorty-fail-sql -o custom-columns=NAME:.metadata.name,PHASE:.status.phase,RESTARTS:.status.containerStatuses[0].restartCount,EXITCODE:.status.containerStatuses[0].state.terminated.exitCode

# 事件里能看到每次重试和最终放弃。
kubectl -n shorty describe job shorty-fail-sql | sed -n '/^Events:/,$p' | tail -n 10

# 失败 Pod 的日志也保留着，错误原因直接可读。
kubectl -n shorty logs -l batch.kubernetes.io/job-name=shorty-fail-sql --tail=4 --prefix
