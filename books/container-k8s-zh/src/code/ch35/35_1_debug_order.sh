# 35.1 按“影响面 → 事件 → 状态 → 日志 → 端点”的顺序定位一次 readiness 失败。

# 先确认影响面：Deployment 的两个副本都应该可用，Pod 都应该 Ready。
kubectl -n shorty get deploy api
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide

# 用户从 Gateway 进来。先记下正常时的状态码。
kubectl -n shorty port-forward svc/shorty-gw-nginx 18081:80 >/tmp/ch35-pf-gw.log 2>&1 &
PFG=$!
curl --retry 30 --retry-delay 1 --retry-all-errors -sS -o /dev/null 2>/dev/null \
  -w 'gateway /healthz -> %{http_code}\n' localhost:18081/healthz

# 只挑一个副本，打开它的 not_ready 开关；另一个副本继续服务。
POD=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')
kubectl -n shorty port-forward pod/$POD 18082:8080 >/tmp/ch35-pf-api.log 2>&1 &
PF=$!
curl --retry 30 --retry-delay 1 --retry-all-errors -sS 2>/dev/null -X POST \
  'localhost:18082/admin/fault?not_ready=true'

# 第 1 步：看事件。kubelet 的探针失败会写成 Warning Unhealthy。
kubectl -n shorty wait --for=condition=Ready=false pod/$POD --timeout=90s

# 第 1 步：看事件。kubelet 的探针失败会写成 Warning Unhealthy。
#> expect: Readiness probe failed
kubectl -n shorty get events --field-selector reason=Unhealthy --sort-by=.lastTimestamp

# 影响面变了：可用副本从 2 降到 1。
kubectl -n shorty get deploy api
kubectl -n shorty get pods -l app.kubernetes.io/name=api

# 第 2 步：看 Pod 状态。Conditions 和容器状态会指出是 readiness 而不是进程挂了。
kubectl -n shorty describe pod $POD \
  | grep -E '^[[:space:]]+(State|Ready|Restart Count):|^Conditions:|^[[:space:]]+(Initialized|Ready|ContainersReady|PodScheduled)[[:space:]]'

# 第 3 步：看日志。应用把每次请求都写成 JSON，探针请求也在里面。
kubectl -n shorty logs $POD --tail=100 \
  | jq -c 'select(.path == "/readyz") | {status, path, duration_ms}' | tail -n 3

# 第 4 步：看 Service 的后端。这个副本还在 EndpointSlice 里，但 ready=false。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
  -o jsonpath='{range .items[*].endpoints[*]}{.addresses[0]}{" ready="}{.conditions.ready}{"\n"}{end}'

# 应用自己会说为什么没就绪；这比从 503 猜原因快。
curl -sS localhost:18082/readyz

# 入口仍然返回 200：另一个副本还在。两个都掉出去时才会 502。
curl -sS -o /dev/null -w 'gateway /healthz -> %{http_code}\n' localhost:18081/healthz

# 恢复开关，等 kubelet 重新探到 Ready。
curl -sS -X POST 'localhost:18082/admin/fault?not_ready=false'
kubectl -n shorty wait --for=condition=Ready pod/$POD --timeout=90s
# Pod Ready 之后 Deployment 的 availableReplicas 还要等一个协调周期。
kubectl -n shorty wait --for=jsonpath='{.status.availableReplicas}'=2 \
  deployment/api --timeout=90s
kubectl -n shorty get deploy api
kill $PF $PFG
