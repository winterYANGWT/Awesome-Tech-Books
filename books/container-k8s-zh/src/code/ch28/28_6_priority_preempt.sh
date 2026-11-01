kubectl apply -f priorityclasses.yaml
kubectl get priorityclass
# 算出 worker2 还剩多少可分配的 CPU 请求量，再用低优先级 Pod 把它几乎占满。
alloc=$(kubectl get node shorty-worker2 -o jsonpath='{.status.allocatable.cpu}')
used=$(kubectl describe node shorty-worker2 | awk '/^Allocated resources:/{f=1} f && $1=="cpu"{print $2; exit}')
to_m() { case "$1" in *m) echo "${1%m}";; *) echo $(($1 * 1000));; esac; }
free=$(( $(to_m "$alloc") - $(to_m "$used") ))
filler=$(( free - 200 ))
sed "s/__CPU__/${filler}m/" priority-filler.yaml | kubectl apply -f -
kubectl -n shorty wait --for=condition=Ready pod/fill-worker2 --timeout=120s
kubectl apply -f priority-high.yaml
# 抢占先给受害者打上 deletionTimestamp；轮询到这个瞬间再取状态。
for i in $(seq 1 30); do
  ts=$(kubectl -n shorty get pod fill-worker2 -o jsonpath='{.metadata.deletionTimestamp}')
  [ -n "$ts" ] && break
  sleep 1
done
kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="DisruptionTarget")].reason}'=PreemptionByScheduler pod/fill-worker2 --timeout=30s
kubectl -n shorty get pod fill-worker2 -o wide
kubectl -n shorty get pod fill-worker2 -o jsonpath='{.status.conditions[?(@.type=="DisruptionTarget")].reason}{" "}{.status.conditions[?(@.type=="DisruptionTarget")].message}{"\n"}'
kubectl -n shorty wait --for=condition=Ready pod/high-priority --timeout=120s
kubectl -n shorty get pod high-priority -o wide
#> hide
kubectl -n shorty delete pod fill-worker2 high-priority --force --grace-period=0 --wait=false --ignore-not-found
