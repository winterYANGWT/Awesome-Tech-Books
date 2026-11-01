# 29.4 给 HPA 加缩容行为：稳定窗口 120 秒，每 30 秒最多删 1 个 Pod。
kubectl apply -f hpa-api-behavior.yaml

# 确认 behavior 真的写进去了。
kubectl -n shorty get hpa api -o yaml | sed -n '/^  behavior:/,/^  metrics:/p'

# 先把副本收回 2 个，避免从 3 个副本起压时一步算到 maxReplicas。
kubectl -n shorty scale deployment api --replicas=2
kubectl -n shorty rollout status deployment/api --timeout=180s

# 起同样的压测，等 HPA 扩到 4 个。
kubectl delete job loadgen -n shorty --ignore-not-found
kubectl apply -f loadgen-job.yaml
kubectl -n shorty wait --for=condition=Ready pod -l app.kubernetes.io/name=loadgen --timeout=120s
for i in $(seq 1 24); do
  current=$(kubectl -n shorty get hpa api -o jsonpath='{.status.currentReplicas}')
  echo "scale-up current=$current"
  [ "$current" = "4" ] && break
  sleep 5
done

# 让 HPA 在 4 个副本上多同步几轮，缩容历史里才会记下 4 这个推荐值。
#> hide
for i in $(seq 1 6); do
  read -r current desired cpu < <(kubectl -n shorty get hpa api \
    -o jsonpath='{.status.currentReplicas} {.status.desiredReplicas} {.status.currentMetrics[0].resource.current.averageUtilization}')
  printf 'hold current=%s desired=%s cpu=%s%%\n' "$current" "$desired" "${cpu:-unknown}"
  sleep 10
done

# 停掉压测，从这一刻开始记录副本数。
kubectl delete job loadgen -n shorty --wait=true
start=$SECONDS
#> timeout: 300
for i in $(seq 1 14); do
  read -r current desired cpu < <(kubectl -n shorty get hpa api \
    -o jsonpath='{.status.currentReplicas} {.status.desiredReplicas} {.status.currentMetrics[0].resource.current.averageUtilization}')
  printf 't=%03ds current=%s desired=%s cpu=%s%%\n' "$((SECONDS - start))" "$current" "$desired" "${cpu:-unknown}"
  [ "$current" = "2" ] && break
  sleep 20
done

kubectl get hpa api -n shorty
