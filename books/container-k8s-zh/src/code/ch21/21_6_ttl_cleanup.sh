# TTL 设成 30 秒：先看它完成，再看它消失。
kubectl apply -f job-ttl.yaml
kubectl -n shorty wait --for=condition=complete job/shorty-migrate-ttl --timeout=120s
kubectl -n shorty get job shorty-migrate-ttl -o custom-columns=NAME:.metadata.name,COMPLETIONS:.status.succeeded,TTL:.spec.ttlSecondsAfterFinished,COMPLETIONTIME:.status.completionTime
kubectl -n shorty logs job/shorty-migrate-ttl

# 轮询到 Job 被 TTL 控制器删除；不用 -w，避免滚动输出。
#> timeout: 120
start=$(date +%s)
for i in $(seq 1 90); do
  if ! kubectl -n shorty get job shorty-migrate-ttl >/dev/null 2>&1; then
    echo "job deleted after $(( $(date +%s) - start ))s"
    break
  fi
  sleep 1
done

# Job 删掉后，它创建的 Pod 也一起被级联删除。
#> expect: No resources found
kubectl -n shorty get jobs -l app.kubernetes.io/name=shorty-migrate-ttl
kubectl -n shorty get pods -l batch.kubernetes.io/job-name=shorty-migrate-ttl
