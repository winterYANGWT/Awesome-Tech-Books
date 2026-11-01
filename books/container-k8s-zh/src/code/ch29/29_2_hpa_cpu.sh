# 29.2 创建 HPA api，给它 2–5 个副本、CPU 目标 50%。
# 压测镜像的源码在 loadgen/main.go，构建文件是 loadgen/Dockerfile，由 setup.sh 构建并加载。
kubectl apply -f hpa-api.yaml
kubectl wait --for=condition=ScalingActive hpa/api -n shorty --timeout=120s
kubectl get hpa api -n shorty

# 起压测客户端，按固定速率访问 api 的 /。
kubectl delete job loadgen -n shorty --ignore-not-found
kubectl apply -f loadgen-job.yaml
kubectl -n shorty wait --for=condition=Ready pod -l app.kubernetes.io/name=loadgen --timeout=120s

# 观察 HPA 的当前副本、期望副本和 CPU 利用率；每 5 秒记一行。
start=$SECONDS
for i in $(seq 1 24); do
  read -r current desired cpu < <(kubectl -n shorty get hpa api \
    -o jsonpath='{.status.currentReplicas} {.status.desiredReplicas} {.status.currentMetrics[0].resource.current.averageUtilization}')
  printf 't=%02ds current=%s desired=%s cpu=%s%%\n' "$((SECONDS - start))" "$current" "$desired" "${cpu:-unknown}"
  [ "$current" = "4" ] && break
  sleep 5
done

kubectl get hpa api -n shorty
kubectl get deployment api -n shorty

# 新 Pod 的指标要等下一次采集才出现；最多等 30 秒，不把示例卡死。
for i in $(seq 1 15); do
  [ "$(kubectl top pods -n shorty -l app.kubernetes.io/name=api --no-headers 2>/dev/null | wc -l)" -eq 4 ] && break
  sleep 2
done

# 每个 Pod 的 CPU 用量；除以 requests.cpu（50m）就是 HPA 看到的利用率。
kubectl top pods -n shorty -l app.kubernetes.io/name=api

# 停止压测。HPA 默认的 5 分钟缩容稳定窗口会让 4 个副本保持一会儿。
kubectl delete job loadgen -n shorty --wait=true
