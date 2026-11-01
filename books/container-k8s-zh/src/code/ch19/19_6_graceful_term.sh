# 先把上一节滚动更新留下的旧 Pod 清掉，只看这次实验的两个副本。
kubectl -n shorty scale deployment/api --replicas=0
kubectl -n shorty wait --for=delete pod -l app.kubernetes.io/name=api --timeout=120s

kubectl -n shorty apply -f api-graceful.yaml
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide

# 客户端通过 Service 连续发 15 个慢请求（每个约 2 秒）。
kubectl -n shorty apply -f grace-client.yaml
kubectl -n shorty wait --for=condition=Ready pod/grace-client --timeout=60s

victim=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api \
    --sort-by=.metadata.creationTimestamp -o name | tail -n 1 | cut -d/ -f2)
echo "准备删除 $victim"

# 先同步取一次历史日志，再在后台接上 follow；这样日志文件一开始就非空。
kubectl -n shorty logs -c api "$victim" > /tmp/terminating.log
kubectl -n shorty logs -f -c api "$victim" >> /tmp/terminating.log 2>&1 &
logpid=$!
kubectl -n shorty delete pod "$victim"
wait "$logpid"

# 只看退出流程的三行：readiness 置败、等待、关闭 HTTP。
#> expect: 收到退出信号
#> expect: 开始关闭 HTTP
grep -E '收到退出信号|开始关闭 HTTP|HTTP 服务已关闭' /tmp/terminating.log

# 15 个请求全部成功，没有 5xx，也没有连接被拒。
kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Succeeded pod/grace-client --timeout=120s
#> expect: fail=0
kubectl -n shorty logs grace-client

kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide

# 恢复本章终点的状态：先清空，再 apply 1 个副本的 api + metrics-scraper。
kubectl -n shorty scale deployment/api --replicas=0
kubectl -n shorty wait --for=delete pod -l app.kubernetes.io/name=api --timeout=120s
kubectl -n shorty apply -f api-sidecar.yaml
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty delete pod grace-client
kubectl -n shorty get pods -l app.kubernetes.io/name=api
