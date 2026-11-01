# 4 个完成、最多 2 个同时运行。
kubectl apply -f job-parallel.yaml

# 等第一批 Pod 出现，再开始采样；不用 -w，避免滚动输出。
#> hide
for i in $(seq 1 60); do
  active=$(kubectl -n shorty get job shorty-parallel -o jsonpath='{.status.active}' 2>/dev/null)
  if [ -n "$active" ] && [ "$active" -gt 0 ]; then break; fi
  sleep 1
done

# 每秒读一次 status.active，记录整个 Job 生命周期里的最大值。
#> timeout: 120
#> expect: max active pods: 2
max=0; while :; do
  active=$(kubectl -n shorty get job shorty-parallel -o jsonpath='{.status.active}' 2>/dev/null)
  active=${active:-0}
  [ "$active" -gt "$max" ] && max=$active
  complete=$(kubectl -n shorty get job shorty-parallel -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}' 2>/dev/null)
  [ "$complete" = True ] && break
  sleep 1
done; echo "max active pods: $max"

# Job 最终完成 4 个；completedIndexes 说明每个 index 都有一个成功 Pod。
kubectl -n shorty get job shorty-parallel
kubectl -n shorty get job shorty-parallel -o jsonpath='completedIndexes={.status.completedIndexes}{"\n"}'

# Pod 名里带 index，日志里也能看到每个 worker 拿到的是哪个编号。
kubectl -n shorty get pods -l batch.kubernetes.io/job-name=shorty-parallel -o custom-columns=NAME:.metadata.name,PHASE:.status.phase
kubectl -n shorty logs -l batch.kubernetes.io/job-name=shorty-parallel --prefix | sort
