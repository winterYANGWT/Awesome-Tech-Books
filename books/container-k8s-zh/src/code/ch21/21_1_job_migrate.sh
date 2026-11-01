# 21.1 看 stage 自带的迁移 Job：它已经跑完，Pod 仍保留。
kubectl -n shorty get job shorty-migrate -o custom-columns=NAME:.metadata.name,COMPLETIONS:.spec.completions,PARALLELISM:.spec.parallelism,BACKOFF:.spec.backoffLimit,SUSPEND:.spec.suspend,ACTIVE:.status.active,SUCCEEDED:.status.succeeded,FAILED:.status.failed

# Job 和它创建的 Pod 放在一起看：Job 是 Complete，Pod 是 Completed。
kubectl -n shorty get job,pod -l app.kubernetes.io/name=shorty-migrate

# 完成的 Job 仍然可以用 kubectl logs 读到 Pod 的日志。
kubectl -n shorty logs job/shorty-migrate

# kubectl wait 只等条件，不猜时间。
kubectl -n shorty wait --for=condition=complete job/shorty-migrate --timeout=10s

# 终态写在 status.conditions 里。
kubectl -n shorty get job shorty-migrate -o jsonpath='{range .status.conditions[*]}{.type}={.status} reason={.reason}{"\n"}{end}'
